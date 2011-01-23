#include "databasecommand_resolve.h"

#include "album.h"
#include "sourcelist.h"

#define MINSCORE 0.5

using namespace Tomahawk;


DatabaseCommand_Resolve::DatabaseCommand_Resolve( const QVariant& v, bool searchlocal )
    : DatabaseCommand()
    , m_v( v )
    , m_searchlocal( searchlocal )
{
}


void
DatabaseCommand_Resolve::exec( DatabaseImpl* lib )
{
    const QMap<QString, QVariant> map = m_v.toMap();

    const Tomahawk::QID qid = map.value( "qid" ).toString();
    const QString artistname = map.value( "artist" ).toString();
    const QString albumname  = map.value( "album" ).toString();
    const QString trackname  = map.value( "track" ).toString();
    const QString resulthint = map.value( "resulthint" ).toString();

    collection_ptr coll;
    QList<Tomahawk::result_ptr> res;
    if ( !resulthint.isEmpty() )
    {
        qDebug() << "Using result-hint to speed up resolving:" << resulthint;

        QVariantMap m = lib->result( resulthint );
        if ( !m.isEmpty() )
        {
            if ( m.value( "srcid" ).toUInt() > 0 )
            {
                source_ptr s = SourceList::instance()->get( m.value( "srcid" ).toUInt() );
                if ( !s.isNull() )
                    coll = s->collection();
            }
            else
                coll = SourceList::instance()->getLocal()->collection();

            if ( !coll.isNull() )
            {
                res << Tomahawk::result_ptr( new Tomahawk::Result( m, coll ) );
                emit results( qid, res );

                return;
            }
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
    QList< int > artists = lib->searchTable( "artist", artistname, 10 );
    QList< int > tracks  = lib->searchTable( "track", trackname, 10 );
    QList< int > albums  = lib->searchTable( "album", albumname, 10 );

    if( artists.length() == 0 || tracks.length() == 0 )
    {
        //qDebug() << "No candidates found in first pass, aborting resolve" << artistname << trackname;
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
                            "file.source %1 AND "
                            "file.id = file_join.file AND "
                            "file_join.artist IN (%2) AND "
                            "file_join.track IN (%3)"
        ).arg( m_searchlocal ? "IS NULL" : " > 0 " )
         .arg( artsl.join( "," ) )
         .arg( trksl.join( "," ) );

    files_query.prepare( sql );
    files_query.exec();

    while( files_query.next() )
    {
        QVariantMap m;

        m["mtime"]    = files_query.value( 1 ).toString();
        m["size"]     = files_query.value( 2 ).toInt();
        m["hash"]     = files_query.value( 3 ).toString();
        m["mimetype"] = files_query.value( 4 ).toString();
        m["duration"] = files_query.value( 5 ).toInt();
        m["bitrate"]  = files_query.value( 6 ).toInt();
        m["artist"]   = files_query.value( 10 ).toString();
        m["artistid"] = files_query.value( 15 ).toUInt();
        m["album"]    = files_query.value( 11 ).toString();
        m["albumid"]  = files_query.value( 16 ).toUInt();
        m["track"]    = files_query.value( 12 ).toString();
        m["srcid"]    = files_query.value( 13 ).toInt();
        m["albumpos"] = files_query.value( 14 ).toUInt();
        m["sid"]      = uuid();

        source_ptr s;
        const QString url_str = files_query.value( 0 ).toString();
        if( m_searchlocal )
        {
            s = SourceList::instance()->getLocal();
            m["url"] = url_str;
            m["source"] = "Local Database"; // TODO
        }
        else
        {
            s = SourceList::instance()->get( files_query.value( 13 ).toUInt() );
            if( s.isNull() )
            {
                //qDebug() << "Skipping result for offline sourceid:" << files_query.value( 13 ).toUInt();
                // will happen for valid sources which are offline (and thus not in the sourcelist)
                continue;
            }

            m.insert( "url", QString( "servent://%1\t%2" ).arg( s->userName() ).arg( url_str ) );
            m.insert( "source", s->friendlyName() );
        }

        float score = how_similar( m_v.toMap(), m );
        //qDebug() << "Score calc:" << timer.elapsed() << "ms";

        m["score"] = score;
        if( score < MINSCORE )
            continue;

        coll = s->collection();
        res << Tomahawk::result_ptr( new Tomahawk::Result( m, coll ) );
    }

    emit results( qid, res );
}


// TODO make clever (ft. featuring live (stuff) etc)
float
DatabaseCommand_Resolve::how_similar( const QVariantMap& q, const QVariantMap& r )
{
    // query values
    const QString qArtistname = DatabaseImpl::sortname( q.value( "artist" ).toString() );
    const QString qAlbumname  = DatabaseImpl::sortname( q.value( "album" ).toString() );
    const QString qTrackname  = DatabaseImpl::sortname( q.value( "track" ).toString() );

    // result values
    const QString rArtistname = DatabaseImpl::sortname( r.value( "artist" ).toString() );
    const QString rAlbumname  = DatabaseImpl::sortname( r.value( "album" ).toString() );
    const QString rTrackname  = DatabaseImpl::sortname( r.value( "track" ).toString() );

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
