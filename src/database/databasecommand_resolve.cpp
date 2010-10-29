#include "databasecommand_resolve.h"

#include "tomahawk/tomahawkapp.h"

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
    const Tomahawk::QID qid = m_v.toMap().value("qid").toString();
    const QString artistname = m_v.toMap().value("artist").toString();
    const QString albumname  = m_v.toMap().value("album").toString();
    const QString trackname  = m_v.toMap().value("track").toString();

    //qDebug() << Q_FUNC_INFO << artistname << trackname;

    /*
        Resolving is a 2 stage process.
        1) find list of trk/art/alb IDs that are reasonable matches to the metadata given
        2) find files in database by permitted sources and calculate score, ignoring
           results that are less than MINSCORE
     */

    typedef QPair<int,float> scorepair_t;

    // STEP 1
    QList< int > artists = lib->searchTable( "artist", artistname, 10 );
    QList< int > tracks  = lib->searchTable( "track",  trackname, 10 );
    QList< int > albums  = lib->searchTable( "album",  albumname, 10 );

    //qDebug() << "art" << artists.size() << "trk" << tracks.size();
    //qDebug() << "searchTable calls duration:" << timer.elapsed();

    if( artists.length() == 0 || tracks.length() == 0 )
    {
        //qDebug() << "No candidates found in first pass, aborting resolve" << artistname << trackname;
        return;
    }

    // STEP 2
    TomahawkSqlQuery files_query = lib->newquery();

    QStringList artsl, trksl;
    foreach( int i, artists ) artsl.append( QString::number(i) );
    foreach( int i, tracks  ) trksl.append( QString::number(i) );

    QString sql = QString( "SELECT "
                            "url, mtime, size, md5, mimetype, duration, bitrate, file_join.artist, file_join.album, file_join.track, "
                            "artist.name as artname, "
                            "album.name as albname, "
                            "track.name as trkname, "
                            "file.source, "
                            "file_join.albumpos "
                            "FROM file, file_join, artist, track "
                            "LEFT JOIN album ON album.id = file_join.album "
                            "WHERE "
                            "artist.id = file_join.artist AND "
                            "track.id = file_join.track AND "
                            "file.source %1 AND "
                            "file.id = file_join.file AND "
                            "file_join.artist IN (%2) AND "
                            "file_join.track IN (%3) "
                            "ORDER by file_join.artist,file_join.track"
        ).arg( m_searchlocal ? "IS NULL" : " IN (SELECT id FROM source WHERE isonline = 'true') " )
         .arg( artsl.join(",") )
         .arg( trksl.join(",") );

    files_query.prepare( sql );
    bool ok = files_query.exec();
    if(!ok)
        throw "Error";

    //qDebug() << "SQL exec() duration, ms, " << timer.elapsed()
    //         << "numresults" << files_query.numRowsAffected();
    //qDebug() << sql;

    QList<Tomahawk::result_ptr> res;

    while( files_query.next() )
    {
        QVariantMap m;

        m["mtime"]    = files_query.value(1).toString();
        m["size"]     = files_query.value(2).toInt();
        m["hash"]     = files_query.value(3).toString();
        m["mimetype"] = files_query.value(4).toString();
        m["duration"] = files_query.value(5).toInt();
        m["bitrate"]  = files_query.value(6).toInt();
        m["artist"]   = files_query.value(10).toString();
        m["album"]    = files_query.value(11).toString();
        m["track"]    = files_query.value(12).toString();
        m["srcid"]    = files_query.value(13).toInt();
        m["albumpos"] = files_query.value(14).toUInt();
        m["sid"]      = uuid();

        collection_ptr coll;

        const QString url_str = files_query.value( 0 ).toString();
        if( m_searchlocal )
        {
            coll = APP->sourcelist().getLocal()->collection();
            m["url"] = url_str;
            m["source"] = "Local Database"; // TODO
        }
        else
        {
            source_ptr s = APP->sourcelist().lookup( files_query.value( 13 ).toUInt() );
            if( s.isNull() )
            {
                //qDebug() << "Skipping result for offline sourceid:" << files_query.value(13).toUInt();
                // will happen for valid sources which are offline (and thus not in the sourcelist)
                return;
            }

            coll = s->collection();
            m.insert( "url", QString( "servent://%1\t%2" )
                                .arg( s->userName() )
                                .arg( url_str ) );
            m.insert( "source", s->friendlyName() );
        }

        //int artid = files_query.value( 7 ).toInt();
        //int albid = files_query.value( 8 ).toInt();
        //int trkid = files_query.value( 9 ).toInt();

        float score = how_similar( m_v.toMap(), m );
        //qDebug() << "Score calc:" << timer.elapsed();

        m["score"] = score;
        //qDebug() << "RESULT" << score << m;

        if( score < MINSCORE )
            continue;

        res << Tomahawk::result_ptr( new Tomahawk::Result( m, coll ) );
    }

    // return results, if any found
    if( res.length() > 0 )
    {
        emit results( qid, res );
    }
}


// TODO make clever (ft. featuring live (stuff) etc)
float
DatabaseCommand_Resolve::how_similar( const QVariantMap& q, const QVariantMap& r )
{
    // query values
    const QString qArtistname = DatabaseImpl::sortname( q.value("artist").toString() );
    const QString qAlbumname  = DatabaseImpl::sortname( q.value("album").toString() );
    const QString qTrackname  = DatabaseImpl::sortname( q.value("track").toString() );

    // result values
    const QString rArtistname = DatabaseImpl::sortname( r.value("artist").toString() );
    const QString rAlbumname  = DatabaseImpl::sortname( r.value("album").toString() );
    const QString rTrackname  = DatabaseImpl::sortname( r.value("track").toString() );

    // normal edit distance
    int artdist = levenshtein( qArtistname, rArtistname );
    int albdist = levenshtein( qAlbumname,  rAlbumname );
    int trkdist = levenshtein( qTrackname,  rTrackname );

    // max length of name
    int mlart = qMax( qArtistname.length(), rArtistname.length() );
    int mlalb = qMax( qAlbumname.length(),  rAlbumname.length() );
    int mltrk = qMax( qTrackname.length(),  rTrackname.length() );

    // distance scores
    float dcart = (float)( mlart - artdist ) / mlart;
    float dcalb = (float)( mlalb - albdist ) / mlalb;
    float dctrk = (float)( mltrk - trkdist ) / mltrk;

    // don't penalize for missing album name
    if( qAlbumname.length() == 0 ) dcalb = 1.0;

    // weighted, so album match is worth less than track title
    float combined = ( dcart*4 + dcalb + dctrk*5 ) / 10;

    return combined;
}
