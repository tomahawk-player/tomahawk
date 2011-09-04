/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Michael Zanetti <mzanetti@kde.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "dropjob.h"

#include "artist.h"
#include "album.h"
#include "source.h"

#include "utils/spotifyparser.h"
#include "utils/rdioparser.h"
#include "utils/shortenedlinkparser.h"
#include "utils/logger.h"
#include "globalactionmanager.h"
#include "infosystem/infosystem.h"
#include "utils/xspfloader.h"
using namespace Tomahawk;

DropJob::DropJob( QObject *parent )
    : QObject( parent )
    , m_queryCount( 0 )
{
}

DropJob::~DropJob()
{
    qDebug() << "destryong DropJob";
}

/// QMIMEDATA HANDLING

QStringList
DropJob::mimeTypes()
{
    QStringList mimeTypes;
    mimeTypes << "application/tomahawk.query.list"
              << "application/tomahawk.plentry.list"
              << "application/tomahawk.result.list"
              << "application/tomahawk.result"
              << "application/tomahawk.metadata.artist"
              << "application/tomahawk.metadata.album"
              << "application/tomahawk.mixed"
              << "text/plain";
    return mimeTypes;
}

bool
DropJob::DropAction()
{

    if ( dropAction() == DropJob::Create )
        return true;
    return false;

}


bool
DropJob::acceptsMimeData( const QMimeData* data, DropJob::DropTypes acceptedType, DropJob::DropActions acceptedAction)
{


    if (acceptedType.testFlag(DropJob::None))
          qDebug() << Q_FUNC_INFO << "AcceptedType is None";

    if (acceptedType.testFlag(DropJob::All))
          qDebug() << Q_FUNC_INFO << "AcceptedType is All";

    if (acceptedType.testFlag(DropJob::Playlist))
          qDebug() << Q_FUNC_INFO << "AcceptedType is Playlist";

    if (acceptedType.testFlag(DropJob::Track))
          qDebug() << Q_FUNC_INFO << "AcceptedType is Track";

    if (acceptedAction.testFlag(DropJob::Append))
          qDebug() << Q_FUNC_INFO << "AcceptedAction is Append";

    if (acceptedAction.testFlag(DropJob::Create))
          qDebug() << Q_FUNC_INFO << "AcceptedAction is Create";

    if ( data->hasFormat( "application/tomahawk.query.list" )
        || data->hasFormat( "application/tomahawk.plentry.list" )
        || data->hasFormat( "application/tomahawk.result.list" )
        || data->hasFormat( "application/tomahawk.result" )
        || data->hasFormat( "application/tomahawk.mixed" )
        || data->hasFormat( "application/tomahawk.metadata.album" )
        || data->hasFormat( "application/tomahawk.metadata.artist" ) )
    {
        return true;
    }

    if( data->hasFormat( "text/plain" )
        && data->data( "text/plain" ).contains( "xspf" )
        && ( acceptedType.testFlag(DropJob::Playlist) || acceptedType.testFlag(DropJob::All) )
        && acceptedAction.testFlag(DropJob::Create)
      )
        return true;

    // crude check for spotify tracks
    if ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "spotify" )
         && data->data( "text/plain" ).contains( "playlist" )
         && ( acceptedType.testFlag(DropJob::Playlist) || acceptedType.testFlag(DropJob::All) )
       )
              return true;

    // crude check for spotify tracks
    if ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "spotify" )
         && data->data( "text/plain" ).contains( "track" )
         && ( acceptedType.testFlag(DropJob::Track) || acceptedType.testFlag(DropJob::All) )
       )
        return true;

    // crude check for rdio tracks
    if ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "rdio.com" )
         && data->data( "text/plain" ).contains( "track" )
         && ( acceptedType.testFlag(DropJob::Track) || acceptedType.testFlag(DropJob::All) )
       )
        return true;


    // We whitelist t.co and bit.ly (and j.mp) since they do some link checking. Often playable (e.g. spotify..) links hide behind them,
    //  so we do an extra level of lookup
    if ( ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "bit.ly" ) ) ||
         ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "j.mp" ) ) ||
         ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "t.co" ) ) ||
         ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "rd.io" ) )
         && ( acceptedType.testFlag(DropJob::Track) || acceptedType.testFlag(DropJob::All) ) )
        return true;

    return false;
}

void
DropJob::setGetWholeArtists( bool getWholeArtists )
{
    m_getWholeArtists = getWholeArtists;
}

void
DropJob::setGetWholeAlbums( bool getWholeAlbums )
{
    m_getWholeAlbums = getWholeAlbums;
}


void
DropJob::tracksFromMimeData( const QMimeData* data, bool allowDuplicates, bool onlyLocal, bool top10 )
{
    m_allowDuplicates = allowDuplicates;
    m_onlyLocal = onlyLocal;
    m_top10 = top10;

    parseMimeData( data );

    if ( m_queryCount == 0 )
    {
        if ( onlyLocal )
            removeRemoteSources();

        if ( !allowDuplicates )
            removeDuplicates();

        emit tracks( m_resultList );
        deleteLater();
    }
}

void
DropJob::parseMimeData( const QMimeData *data )
{


    if(dropTypes() & DropJob::Playlist)
        qDebug() << Q_FUNC_INFO << "DropType is Playlist";
    if(dropTypes() & DropJob::All)
        qDebug() << Q_FUNC_INFO << "DropType is All";
    if(dropTypes() & DropJob::Track)
        qDebug() << Q_FUNC_INFO << "DropType is Track";
    if(dropTypes() & DropJob::Artist)
        qDebug() << Q_FUNC_INFO << "DropType is Artist";
    if(DropTypes() & DropJob::None)
        qDebug() << Q_FUNC_INFO << "DropType is None";
    else qDebug() << "DropType is I DONT KNOW!";

    if(dropAction() == DropJob::Append)
            qDebug() << Q_FUNC_INFO << "DropAction is Append";
    if(dropAction() == DropJob::Create)
            qDebug() << Q_FUNC_INFO << "DropAction is Create";


    QList< query_ptr > results;
    if ( data->hasFormat( "application/tomahawk.query.list" ) )
        results = tracksFromQueryList( data );
    else if ( data->hasFormat( "application/tomahawk.result.list" ) )
        results = tracksFromResultList( data );
    else if ( data->hasFormat( "application/tomahawk.metadata.album" ) )
        results = tracksFromAlbumMetaData( data );
    else if ( data->hasFormat( "application/tomahawk.metadata.artist" ) )
        results = tracksFromArtistMetaData( data );
    else if ( data->hasFormat( "application/tomahawk.mixed" ) )
        tracksFromMixedData( data );
    else if ( data->hasFormat( "text/plain" ) )
    {
        QString plainData = QString::fromUtf8( data->data( "text/plain" ).constData() );
        qDebug() << Q_FUNC_INFO << "Got text/plain mime data:" << data->data( "text/plain" ) << "decoded to:" << plainData.trimmed();

        if ( data->hasFormat( "text/plain" ) &&  data->data( "text/plain" ).contains( "xspf" ) )
            handleXspf( data->data( "text/plain" ).trimmed(), DropAction() );
        else if ( plainData.contains( "spotify" ) && plainData.contains( "playlist" ) )
            handleSpPlaylist( plainData, DropAction() );
        else
            handleTrackUrls ( plainData );
    }

    m_resultList.append( results );
}

QList< query_ptr >
DropJob::tracksFromQueryList( const QMimeData* data )
{
    QList< query_ptr > queries;
    QByteArray itemData = data->data( "application/tomahawk.query.list" );
    QDataStream stream( &itemData, QIODevice::ReadOnly );

    while ( !stream.atEnd() )
    {
        qlonglong qptr;
        stream >> qptr;

        query_ptr* query = reinterpret_cast<query_ptr*>(qptr);
        if ( query && !query->isNull() )
        {
            tDebug() << "Dropped query item:" << query->data()->artist() << "-" << query->data()->track();

            if ( m_top10 )
            {
                getTopTen( query->data()->artist() );
            }
            else if ( m_getWholeArtists )
            {
                queries << getArtist( query->data()->artist() );
            }
            else if ( m_getWholeAlbums )
            {
                queries << getAlbum( query->data()->artist(), query->data()->album() );
            }
            else
            {
                queries << *query;
            }
        }
    }

    return queries;
}

QList< query_ptr >
DropJob::tracksFromResultList( const QMimeData* data )
{
    QList< query_ptr > queries;
    QByteArray itemData = data->data( "application/tomahawk.result.list" );
    QDataStream stream( &itemData, QIODevice::ReadOnly );

    while ( !stream.atEnd() )
    {
        qlonglong qptr;
        stream >> qptr;

        result_ptr* result = reinterpret_cast<result_ptr*>(qptr);
        if ( result && !result->isNull() )
        {
            tDebug() << "Dropped result item:" << result->data()->artist()->name() << "-" << result->data()->track();
            query_ptr q = result->data()->toQuery();

            if ( m_top10 )
            {
                getTopTen( q->artist() );
            }
            else if ( m_getWholeArtists )
            {
                queries << getArtist( q->artist() );
            }
            else if ( m_getWholeAlbums )
            {
                queries << getAlbum( q->artist(), q->album() );
            }
            else
            {
                q->addResults( QList< result_ptr >() << *result );
                queries << q;
            }
        }
    }

    return queries;
}

QList< query_ptr >
DropJob::tracksFromAlbumMetaData( const QMimeData *data )
{
    QList<query_ptr> queries;
    QByteArray itemData = data->data( "application/tomahawk.metadata.album" );
    QDataStream stream( &itemData, QIODevice::ReadOnly );

    while ( !stream.atEnd() )
    {
        QString artist;
        stream >> artist;
        QString album;
        stream >> album;

        if ( m_top10 )
            getTopTen( artist );
        else if ( m_getWholeArtists )
            queries << getArtist( artist );
        else
            queries << getAlbum( artist, album );
    }
    return queries;
}

QList< query_ptr >
DropJob::tracksFromArtistMetaData( const QMimeData *data )
{
    QList<query_ptr> queries;
    QByteArray itemData = data->data( "application/tomahawk.metadata.artist" );
    QDataStream stream( &itemData, QIODevice::ReadOnly );

    while ( !stream.atEnd() )
    {
        QString artist;
        stream >> artist;

        if ( !m_top10 )
        {
            queries << getArtist( artist );
        }
        else
        {
            getTopTen( artist );
        }
    }
    return queries;
}

QList< query_ptr >
DropJob::tracksFromMixedData( const QMimeData *data )
{
    QList< query_ptr > queries;
    QByteArray itemData = data->data( "application/tomahawk.mixed" );
    QDataStream stream( &itemData, QIODevice::ReadOnly );

    QString mimeType;

    while ( !stream.atEnd() )
    {
        stream >> mimeType;
        qDebug() << "mimetype is" << mimeType;

        QByteArray singleData;
        QDataStream singleStream( &singleData, QIODevice::WriteOnly );

        QMimeData singleMimeData;
        if ( mimeType == "application/tomahawk.query.list" || mimeType == "application/tomahawk.result.list" )
        {
            qlonglong query;
            stream >> query;
            singleStream << query;
        }
        else if ( mimeType == "application/tomahawk.metadata.album" )
        {
            QString artist;
            stream >> artist;
            singleStream << artist;
            QString album;
            stream >> album;
            singleStream << album;
            qDebug() << "got artist" << artist << "and album" << album;
        }
        else if ( mimeType == "application/tomahawk.metadata.artist" )
        {
            QString artist;
            stream >> artist;
            singleStream << artist;
            qDebug() << "got artist" << artist;
        }

        singleMimeData.setData( mimeType, singleData );
        parseMimeData( &singleMimeData );
    }
    return queries;
}

void
DropJob::handleXspf( const QString& fileUrl, bool createNewPlaylist )
{
    qDebug() << Q_FUNC_INFO << "Got xspf playlist!!" << fileUrl;

        // Doing like so on *nix, dont know really how files are
        // passed on others.
        qDebug() << "Got xspf playlist!!";
        QString newFile = fileUrl;
        newFile.replace("file://", "");
        QFile xspfFile(newFile);
        XSPFLoader* l = new XSPFLoader( createNewPlaylist, this );
        tDebug( LOGINFO ) << "Loading local xspf:" << newFile;
        l->load( xspfFile );

}

void
DropJob::handleSpPlaylist( const QString& url, bool createNewPlaylist)
{

    qDebug() << "Got spotify playlist!!" << url;

       if ( url.contains( "open.spotify.com/user") ||
            url.contains( "spotify:user" ) )
       {
         // Lets create a valid playlist uri
         QString playlistUri = url;
         QString validUri;

         if(url.contains( "open.spotify.com/user")){
             playlistUri.replace("http://open.spotify.com/", "");
             QStringList playlist = playlistUri.split( "/" );
             validUri = "spotify:" + playlist.join(":");
         }else validUri = playlistUri;

         tDebug() << "Got a spotify playlist in dropjob!" << validUri;
         SpotifyParser* spot = new SpotifyParser( validUri, this, createNewPlaylist);

         //This currently supports draging and dropping a spotify playlist
         if(createNewPlaylist){
             qDebug() << Q_FUNC_INFO << "Got spotify playlist!! Create new" << url;
         }else{
              qDebug() << Q_FUNC_INFO << "Got spotify playlist!!" << url;
              connect( spot, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
         }

         m_queryCount++;
       }

}

void
DropJob::handleTrackUrls( const QString& urls )
{
    qDebug() << Q_FUNC_INFO << urls;


    if ( urls.contains( "open.spotify.com/user") ||
         urls.contains( "spotify:user" ) )
            handleSpPlaylist( urls, dropAction() );

    else if ( urls.contains( "open.spotify.com/track") ||
         urls.contains( "spotify:track" ) )
    {
        QStringList tracks = urls.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        tDebug() << "Got a list of spotify urls!" << tracks;
        SpotifyParser* spot = new SpotifyParser( tracks, this );
        connect( spot, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;
    } else if ( urls.contains( "rdio.com" ) )
    {
        QStringList tracks = urls.split( "\n" );

        tDebug() << "Got a list of rdio urls!" << tracks;
        RdioParser* rdio = new RdioParser( this );
        connect( rdio, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;

        rdio->parse( tracks );
    } else if ( urls.contains( "bit.ly" ) ||
                urls.contains( "j.mp" ) ||
                urls.contains( "t.co" ) ||
                urls.contains( "rd.io" ) )
    {
        QStringList tracks = urls.split( "\n" );

        tDebug() << "Got a list of shortened urls!" << tracks;
        ShortenedLinkParser* parser = new ShortenedLinkParser( tracks, this );
        connect( parser, SIGNAL( urls( QStringList ) ), this, SLOT( expandedUrls( QStringList ) ) );
        m_queryCount++;
    }
}

void
DropJob::expandedUrls( QStringList urls )
{
    m_queryCount--;
    handleTrackUrls( urls.join( "\n" ) );
}

void
DropJob::onTracksAdded( const QList<Tomahawk::query_ptr>& tracksList )
{
    m_resultList.append( tracksList );

    if ( --m_queryCount == 0 )
    {
        if ( m_onlyLocal )
            removeRemoteSources();

        if ( !m_allowDuplicates )
            removeDuplicates();

        emit tracks( m_resultList );
        deleteLater();
    }
}

void
DropJob::removeDuplicates()
{
    QList< Tomahawk::query_ptr > list;
    foreach ( const Tomahawk::query_ptr& item, m_resultList )
    {
        bool contains = false;
        foreach( const Tomahawk::query_ptr &tmpItem, list )
            if ( item->album() == tmpItem->album()
                 && item->artist() == tmpItem->artist()
                 && item->track() == tmpItem->track() )
                contains = true;
        if ( !contains )
            list.append( item );
    }
    m_resultList = list;
}

void
DropJob::removeRemoteSources()
{
    QList< Tomahawk::query_ptr > list;
    foreach ( const Tomahawk::query_ptr& item, m_resultList )
    {
        bool hasLocalSource = false;
        foreach ( const Tomahawk::result_ptr& result, item->results() )
        {
            if ( !result->collection()->source().isNull() && result->collection()->source()->isLocal() )
                hasLocalSource = true;
        }
        if ( hasLocalSource )
            list.append( item );
    }
    m_resultList = list;
}

void
DropJob::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller == "changeme" )
    {
        Tomahawk::InfoSystem::InfoCriteriaHash artistInfo;

        artistInfo = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();

        QString artist = artistInfo["artist"];

        qDebug() << "Got requestData response for artist" << artist << output;

        QList< query_ptr > results;

        int i = 0;
        foreach ( const QVariant& title, output.toMap().value( "tracks" ).toList() )
        {
            qDebug() << "got title" << title;
            results << Query::get( artist, title.toString(), QString(), uuid() );

            if ( ++i == 10 ) // Only getting top ten for now. Would make sense to make it configurable
                break;
        }

        onTracksAdded( results );
    }
}

QList< query_ptr >
DropJob::getArtist( const QString &artist )
{
    artist_ptr artistPtr = Artist::get( artist );
    if ( artistPtr->tracks().isEmpty() )
    {
        connect( artistPtr.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                                 SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );
        m_queryCount++;
        return QList< query_ptr >();
    }
    else
        return artistPtr->tracks();
}

QList< query_ptr >
DropJob::getAlbum(const QString &artist, const QString &album)
{
    artist_ptr artistPtr = Artist::get( artist );
    album_ptr albumPtr = Album::get( artistPtr, album );

    if ( albumPtr.isNull() )
        return QList< query_ptr >();

    if ( albumPtr->tracks().isEmpty() )
    {
        connect( albumPtr.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                                 SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );
        m_queryCount++;
        return QList< query_ptr >();
    }
    else
        return albumPtr->tracks();
}

void
DropJob::getTopTen( const QString &artist )
{
    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    Tomahawk::InfoSystem::InfoCriteriaHash artistInfo;
    artistInfo["artist"] = artist;

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = "changeme";
    requestData.customData = QVariantMap();

    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( artistInfo );

    requestData.type = Tomahawk::InfoSystem::InfoArtistSongs;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    m_queryCount++;

}
