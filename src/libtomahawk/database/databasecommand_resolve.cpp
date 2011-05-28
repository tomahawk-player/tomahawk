/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "databasecommand_resolve.h"

#include "album.h"
#include "sourcelist.h"

#define MINSCORE 0.5

using namespace Tomahawk;


DatabaseCommand_Resolve::DatabaseCommand_Resolve( const query_ptr& query )
    : DatabaseCommand()
    , m_query( query )
{
}


void
DatabaseCommand_Resolve::exec( DatabaseImpl* lib )
{
    QList<Tomahawk::result_ptr> res;
    if ( !m_query->resultHint().isEmpty() )
    {
        qDebug() << "Using result-hint to speed up resolving:" << m_query->resultHint();

        Tomahawk::result_ptr result = lib->resultFromHint( m_query );
        if ( !result.isNull() && result->collection()->source()->isOnline() )
        {
            res << result;
            emit results( m_query->id(), res );
            return;
        }
    }

    /*
        Resolving is a 2 stage process.
        1) find list of trk/art/alb IDs that are reasonable matches to the metadata given
        2) find files in database by permitted sources and calculate score, ignoring
           results that are less than MINSCORE
     */

    typedef QPair<int, float> scorepair_t;

    // STEP 1
    QList< int > artists = lib->searchTable( "artist", m_query->artist(), 10 );
    QList< int > tracks  = lib->searchTable( "track", m_query->track(), 10 );
    QList< int > albums  = lib->searchTable( "album", m_query->album(), 10 );

    if( artists.length() == 0 || tracks.length() == 0 )
    {
        qDebug() << "No candidates found in first pass, aborting resolve" << m_query->artist() << m_query->track();
        emit results( m_query->id(), res );
        return;
    }

    // STEP 2
    TomahawkSqlQuery files_query = lib->newquery();

    QStringList artsl, trksl;
    foreach( int i, artists )
        artsl.append( QString::number( i ) );
    foreach( int i, tracks )
        trksl.append( QString::number( i ) );

    QString sql = QString( "SELECT "
                            "url, mtime, size, md5, mimetype, duration, bitrate, file_join.artist, file_join.album, file_join.track, "
                            "artist.name as artname, "
                            "album.name as albname, "
                            "track.name as trkname, "
                            "file.source, "
                            "file_join.albumpos, "
                            "artist.id as artid, "
                            "album.id as albid "
                            "FROM file, file_join, artist, track "
                            "LEFT JOIN album ON album.id = file_join.album "
                            "WHERE "
                            "artist.id = file_join.artist AND "
                            "track.id = file_join.track AND "
                            "file.id = file_join.file AND "
                            "file_join.artist IN (%1) AND "
                            "file_join.track IN (%2)" )
         .arg( artsl.join( "," ) )
         .arg( trksl.join( "," ) );

    files_query.prepare( sql );
    files_query.exec();

    while( files_query.next() )
    {
        Tomahawk::result_ptr result( new Tomahawk::Result() );
        source_ptr s;

        const QString url_str = files_query.value( 0 ).toString();
        if( files_query.value( 13 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
            result->setUrl( url_str );
        }
        else
        {
            s = SourceList::instance()->get( files_query.value( 13 ).toUInt() );
            if( s.isNull() )
            {
                qDebug() << "WTF: Could not find source" << files_query.value( 13 ).toUInt();
                continue;
            }

            result->setUrl( QString( "servent://%1\t%2" ).arg( s->userName() ).arg( url_str ) );
        }

        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( files_query.value( 15 ).toUInt(), files_query.value( 10 ).toString() );
        Tomahawk::album_ptr album = Tomahawk::Album::get( files_query.value( 16 ).toUInt(), files_query.value( 11 ).toString(), artist );

        result->setModificationTime( files_query.value( 1 ).toUInt() );
        result->setSize( files_query.value( 2 ).toUInt() );
        result->setMimetype( files_query.value( 4 ).toString() );
        result->setDuration( files_query.value( 5 ).toUInt() );
        result->setBitrate( files_query.value( 6 ).toUInt() );
        result->setArtist( artist );
        result->setAlbum( album );
        result->setTrack( files_query.value( 12 ).toString() );
        result->setRID( uuid() );
        result->setAlbumPos( files_query.value( 14 ).toUInt() );
        result->setId( files_query.value( 9 ).toUInt() );
        result->setYear( files_query.value( 17 ).toUInt() );

        TomahawkSqlQuery attrQuery = lib->newquery();
        QVariantMap attr;

        attrQuery.prepare( "SELECT k, v FROM track_attributes WHERE id = ?" );
        attrQuery.bindValue( 0, result->dbid() );
        attrQuery.exec();
        while ( attrQuery.next() )
        {
            attr[ attrQuery.value( 0 ).toString() ] = attrQuery.value( 1 ).toString();
        }

        result->setAttributes( attr );

        float score = how_similar( m_query, result );
        result->setScore( score );
        if( score < MINSCORE )
            continue;

        result->setCollection( s->collection() );
        res << result;
    }

    emit results( m_query->id(), res );
}


// TODO make clever (ft. featuring live (stuff) etc)
float
DatabaseCommand_Resolve::how_similar( const Tomahawk::query_ptr& q, const Tomahawk::result_ptr& r )
{
    // query values
    const QString qArtistname = DatabaseImpl::sortname( q->artist() );
    const QString qAlbumname  = DatabaseImpl::sortname( q->album() );
    const QString qTrackname  = DatabaseImpl::sortname( q->track() );

    // result values
    const QString rArtistname = DatabaseImpl::sortname( r->artist()->name() );
    const QString rAlbumname  = DatabaseImpl::sortname( r->album()->name() );
    const QString rTrackname  = DatabaseImpl::sortname( r->track() );

    // normal edit distance
    int artdist = levenshtein( qArtistname, rArtistname );
    int albdist = levenshtein( qAlbumname, rAlbumname );
    int trkdist = levenshtein( qTrackname, rTrackname );

    // max length of name
    int mlart = qMax( qArtistname.length(), rArtistname.length() );
    int mlalb = qMax( qAlbumname.length(), rAlbumname.length() );
    int mltrk = qMax( qTrackname.length(), rTrackname.length() );

    // distance scores
    float dcart = (float)( mlart - artdist ) / mlart;
    float dcalb = (float)( mlalb - albdist ) / mlalb;
    float dctrk = (float)( mltrk - trkdist ) / mltrk;

    // don't penalize for missing album name
    if( qAlbumname.length() == 0 )
        dcalb = 1.0;

    // weighted, so album match is worth less than track title
    float combined = ( dcart*4 + dcalb + dctrk*5 ) / 10;
    return combined;
}


int
DatabaseCommand_Resolve::levenshtein( const QString& source, const QString& target )
{
    // Step 1
    const int n = source.length();
    const int m = target.length();

    if ( n == 0 )
        return m;
    if ( m == 0 )
        return n;

    // Good form to declare a TYPEDEF
    typedef QVector< QVector<int> > Tmatrix;
    Tmatrix matrix;
    matrix.resize( n + 1 );

    // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
    // allow for allocation on declaration of 2.nd dimension of vec of vec
    for ( int i = 0; i <= n; i++ )
    {
      QVector<int> tmp;
      tmp.resize( m + 1 );
      matrix.insert( i, tmp );
    }

    // Step 2
    for ( int i = 0; i <= n; i++ )
      matrix[i][0] = i;
    for ( int j = 0; j <= m; j++ )
      matrix[0][j] = j;

    // Step 3
    for ( int i = 1; i <= n; i++ )
    {
      const QChar s_i = source[i - 1];

      // Step 4
      for ( int j = 1; j <= m; j++ )
      {
        const QChar t_j = target[j - 1];

        // Step 5
        int cost;
        if ( s_i == t_j )
          cost = 0;
        else
          cost = 1;

        // Step 6
        const int above = matrix[i - 1][j];
        const int left = matrix[i][j - 1];
        const int diag = matrix[i - 1][j - 1];

        int cell = ( ((left + 1) > (diag + cost)) ? diag + cost : left + 1 );
        if( above + 1 < cell )
            cell = above + 1;

        // Step 6A: Cover transposition, in addition to deletion,
        // insertion and substitution. This step is taken from:
        // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's
        // Enhanced Dynamic Programming ASM Algorithm"
        // (http://www.acm.org/~hlb/publications/asm/asm.html)
        if ( i > 2 && j > 2 )
        {
          int trans = matrix[i - 2][j - 2] + 1;

          if ( source[ i - 2 ] != t_j ) trans++;
          if ( s_i != target[ j - 2 ] ) trans++;
          if ( cell > trans) cell = trans;
        }
        matrix[i][j] = cell;
      }
    }

    // Step 7
    return matrix[n][m];
}
