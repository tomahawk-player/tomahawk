/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Michael Zanetti <mzanetti@kde.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2011-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "DropJob.h"

#include "Artist.h"
#include "Album.h"
#include "Source.h"

#include "utils/SpotifyParser.h"
#include "utils/ItunesParser.h"
#include "utils/RdioParser.h"
#include "utils/M3uLoader.h"
#include "utils/ShortenedLinkParser.h"
#include "utils/SoundcloudParser.h"
#include "utils/ExfmParser.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"
#include "GlobalActionManager.h"
#include "utils/XspfLoader.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "Pipeline.h"

#ifdef QCA2_FOUND
#include "utils/GroovesharkParser.h"
#endif //QCA2_FOUND


using namespace Tomahawk;

bool DropJob::s_canParseSpotifyPlaylists = false;
static QString s_dropJobInfoId = "dropjob";

DropJob::DropJob( QObject *parent )
    : QObject( parent )
    , m_queryCount( 0 )
    , m_onlyLocal( false )
    , m_getWholeArtists( false )
    , m_getWholeAlbums( false )
    , m_top10( false )
    , m_dropAction( Default )
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
              << "text/plain"
              << "text/uri-list";

    return mimeTypes;
}


bool
DropJob::acceptsMimeData( const QMimeData* data, DropJob::DropTypes acceptedType, DropJob::DropAction acceptedAction )
{
    Q_UNUSED( acceptedAction );

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

    // check plain text url types
    if ( !data->hasFormat( "text/plain" ) )
        if ( !data->hasFormat( "text/uri-list" ) )
            return false;


    const QString url = data->data( "text/plain" );
    if ( acceptedType.testFlag( Playlist ) )
    {
        if ( url.contains( "xspf" ) )
            return true;

        if ( url.contains( "m3u" ) )
            return true;

        if ( data->data( "text/uri-list" ).contains( "xspf" ) )
            return true;

        if ( data->data( "text/uri-list" ).contains( "m3u" ) )
            return true;

        // Not the most elegant
        if ( url.contains( "spotify" ) && url.contains( "playlist" ) && s_canParseSpotifyPlaylists )
            return true;

        if( url.contains( "soundcloud" ) && url.contains( "sets" ) )
            return true;

        if( url.contains( "ex.fm" ) && !url.contains( "/song/" ) ) // We treat everything but song as playlist
            return true;

        if ( url.contains( "grooveshark.com" ) && url.contains( "playlist" ) )
            return true;
    }

    if ( acceptedType.testFlag( Track ) )
    {
        if ( url.contains( "m3u" ) )
            return true;

        if ( data->data( "text/uri-list" ).contains( "m3u" ) )
            return true;

        if ( url.contains( "itunes" ) && url.contains( "album" ) ) // YES itunes is fucked up and song links have album/ in the url.
            return true;

        if ( url.contains( "spotify" ) && url.contains( "track" ) )
            return true;

        if ( url.contains( "ex.fm" ) && url.contains( "/song/" ) )
            return true;

        if( url.contains( "soundcloud" ) )
            return true;

        if ( url.contains( "rdio.com" ) && ( ( ( url.contains( "track" ) && url.contains( "artist" ) && url.contains( "album" ) )
                                               || url.contains( "playlists" )  ) ) )
            return true;
    }

    if ( acceptedType.testFlag( Album ) )
    {
        if ( url.contains( "itunes" ) && url.contains( "album" ) ) // YES itunes is fucked up and song links have album/ in the url.
            return true;
        if ( url.contains( "spotify" ) && url.contains( "album" ) )
            return true;
        if ( url.contains( "rdio.com" ) && ( url.contains( "artist" ) && url.contains( "album" ) && !url.contains( "track" ) )  )
            return true;
        if ( url.contains( "ex.fm" ) && url.contains( "site" ) && url.contains( "album" ) )
            return true;
    }

    if ( acceptedType.testFlag( Artist ) )
    {
        if ( url.contains( "itunes" ) && url.contains( "artist" ) ) // YES itunes is fucked up and song links have album/ in the url.
            return true;
        if ( url.contains( "spotify" ) && url.contains( "artist" ) )
            return true;
        if ( url.contains( "rdio.com" ) && ( url.contains( "artist" ) && !url.contains( "album" ) && !url.contains( "track" ) )  )
            return true;
        if( url.contains( "soundcloud" ) )
            return true;
    }

    // We whitelist certain url-shorteners since they do some link checking. Often playable (e.g. spotify) links hide behind them,
    // so we do an extra level of lookup
    if ( ShortenedLinkParser::handlesUrl( url ) )
        return true;

    return false;
}


bool
DropJob::isDropType( DropJob::DropType desired, const QMimeData* data )
{
    const QString url = data->data( "text/plain" );
    if ( desired == Playlist )
    {
        if( url.contains( "xspf" ) || data->data( "text/uri-list").contains( "xspf" ) )
            return true;

        if( url.contains( "m3u" ) || data->data( "text/uri-list").contains( "m3u" ) )
            return true;

        // Not the most elegant
        if ( url.contains( "spotify" ) && url.contains( "playlist" ) && s_canParseSpotifyPlaylists )
            return true;

        if( url.contains( "soundcloud" ) && url.contains( "sets" ) )
            return true;

        if( url.contains( "ex.fm" ) && !url.contains( "/song/" ) ) // We treat all but song as playlist
            return true;

        if ( url.contains( "rdio.com" ) && url.contains( "people" ) && url.contains( "playlist" ) )
            return true;
#ifdef QCA2_FOUND
        if ( url.contains( "grooveshark.com" ) && url.contains( "playlist" ) )
            return true;
#endif //QCA2_FOUND
        if ( ShortenedLinkParser::handlesUrl( url ) )
            return true;
    }

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
DropJob::parseMimeData( const QMimeData* data )
{
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
    else if ( data->hasFormat( "text/plain" ) && !data->data( "text/plain" ).isEmpty() )
    {
        const QString plainData = QString::fromUtf8( data->data( "text/plain" ) );
        handleAllUrls( plainData );

    }
    else if ( data->hasFormat( "text/uri-list" ) )
    {
        const QString plainData = QString::fromUtf8( data->data( "text/uri-list" ).trimmed() );
        handleAllUrls( plainData );
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
            tDebug() << "Dropped query item:" << query->data()->toString();

            if ( m_top10 )
            {
                queries << getTopTen( query->data()->artist() );
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
            queries << getTopTen( artist );
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
            queries << getTopTen( artist );
        }
    }
    return queries;
}


void
DropJob::tracksFromMixedData( const QMimeData *data )
{
    QByteArray itemData = data->data( "application/tomahawk.mixed" );
    QDataStream stream( &itemData, QIODevice::ReadOnly );
    QString mimeType;

    while ( !stream.atEnd() )
    {
        stream >> mimeType;

        QByteArray singleData;
        QDataStream singleStream( &singleData, QIODevice::WriteOnly );

        QMimeData singleMimeData;
        if ( mimeType == "application/tomahawk.query.list" ) 
        {
            qlonglong query;
            stream >> query;
            singleStream << query;
        }
        else if ( mimeType == "application/tomahawk.result.list" )
        {
            qlonglong result;
            stream >> result;
            singleStream << result;
        }
        else if ( mimeType == "application/tomahawk.metadata.album" )
        {
            QString artist;
            stream >> artist;
            singleStream << artist;
            QString album;
            stream >> album;
            singleStream << album;
        }
        else if ( mimeType == "application/tomahawk.metadata.artist" )
        {
            QString artist;
            stream >> artist;
            singleStream << artist;
        }

        singleMimeData.setData( mimeType, singleData );
        parseMimeData( &singleMimeData );
    }
}


void
DropJob::handleM3u( const QString& fileUrls )
{
    tDebug() << Q_FUNC_INFO << "Got M3U playlist!" << fileUrls;
    QStringList urls = fileUrls.split( QRegExp( "\n" ), QString::SkipEmptyParts );

    if ( dropAction() == Default )
        setDropAction( Create );

    tDebug() << "Got a M3U playlist url to parse!" << urls;
    M3uLoader* m = new M3uLoader( urls, dropAction() == Create, this );

    if ( dropAction() == Append )
    {
        tDebug() << Q_FUNC_INFO << "Trying to append contents from" << urls;
        connect( m, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;
    }
    m->parse();
}


void
DropJob::handleXspfs( const QString& fileUrls )
{
    tDebug() << Q_FUNC_INFO << "Got XSPF playlist!" << fileUrls;
    bool error = false;
    QStringList urls = fileUrls.split( QRegExp( "\n" ), QString::SkipEmptyParts );

    if ( dropAction() == Default )
        setDropAction( Create );

    foreach ( const QString& url, urls )
    {
        XSPFLoader* l = 0;
        QFile xspfFile( QUrl::fromUserInput( url ).toLocalFile() );

        if ( xspfFile.exists() )
        {
            l = new XSPFLoader(  dropAction() == Create, this );
            tDebug( LOGINFO ) << "Loading local XSPF" << xspfFile.fileName();
            l->load( xspfFile );
        }
        else if ( QUrl( url ).isValid() )
        {
            l = new XSPFLoader(  dropAction() == Create, this );
            tDebug( LOGINFO ) << "Loading remote XSPF" << url;
            l->load( QUrl( url ) );
        }
        else
        {
            error = true;
            tLog() << "Failed to load or parse dropped XSPF";
        }

        if ( dropAction() == Append && !error && l )
        {
            qDebug() << Q_FUNC_INFO << "Trying to append XSPF";
            connect( l, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
            m_queryCount++;
        }
    }
}


void
DropJob::handleSpotifyUrls( const QString& urlsRaw )
{
    // Todo: Allow search querys, and split these in a better way.
    // Example: spotify:search:artist:Madonna year:<1970 year:>1990
    QStringList urls = urlsRaw.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
    qDebug() << "Got spotify browse uris!" << urls;

    /// Lets allow parsing all spotify uris here, if parse server is not available
    /// fallback to spotify metadata for tracks /hugo
    if ( dropAction() == Default )
        setDropAction( Create );

    tDebug() << "Got a spotify browse uri in dropjob!" << urls;
    SpotifyParser* spot = new SpotifyParser( urls, dropAction() == Create, this );
    spot->setSingleMode( false );

    /// This currently supports draging and dropping a spotify playlist and artist
    if ( dropAction() == Append )
    {
        tDebug() << Q_FUNC_INFO << "Asking for spotify browse contents from" << urls;
        connect( spot, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;
    }
}


void
DropJob::handleRdioUrls( const QString& urlsRaw )
{
    QStringList urls = urlsRaw.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
    qDebug() << "Got Rdio urls!" << urls;

    if ( dropAction() == Default )
        setDropAction( Create );

    RdioParser* rdio = new RdioParser( this );
    connect( rdio, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );

    m_queryCount++;
    rdio->setCreatePlaylist( dropAction() == Create  );
    rdio->parse( urls );
}


void
DropJob::handleSoundcloudUrls( const QString& urlsRaw )
{
    QStringList urls = urlsRaw.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
    qDebug() << "Got Soundcloud urls!" << urls;


    if ( dropAction() == Default )
        setDropAction( Create );

    SoundcloudParser* sc = new SoundcloudParser( urls, dropAction() == Create, this );
    connect( sc, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );

    m_queryCount++;

}


void
DropJob::handleExfmUrls( const QString& urlsRaw )
{
    QStringList urls = urlsRaw.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
    qDebug() << "Got Ex.fm urls!" << urls;


    if ( dropAction() == Default )
        setDropAction( Create );

    ExfmParser* exfm = new ExfmParser( urls, dropAction() == Create, this );
    connect( exfm, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );

    m_queryCount++;

}

void
DropJob::handleGroovesharkUrls ( const QString& urlsRaw )
{
#ifdef QCA2_FOUND
    QStringList urls = urlsRaw.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
    tDebug() << "Got Grooveshark urls!" << urls;

    if ( dropAction() == Default )
        setDropAction( Create );

    GroovesharkParser* groove = new GroovesharkParser( urls, dropAction() == Create, this );
    connect( groove, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );

    if ( dropAction() == Append )
    {
        tDebug() << Q_FUNC_INFO << "Asking for grooveshark contents from" << urls;
        connect( groove, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;
    }
#else
    tLog() << "Tomahawk compiled without QCA support, cannot use groovesharkparser";
#endif
}


void
DropJob::handleAllUrls( const QString& urls )
{
    if ( urls.contains( "xspf" ) )
        handleXspfs( urls );
    else if ( urls.contains( "m3u" ) )
        handleM3u( urls );
    else if ( urls.contains( "spotify" ) /// Handle all the spotify uris on internal server, if not avail. fallback to spotify
              && ( urls.contains( "playlist" ) || urls.contains( "artist" ) || urls.contains( "album" ) || urls.contains( "track" ) )
              && s_canParseSpotifyPlaylists )
        handleSpotifyUrls( urls );
    else if ( urls.contains( "rdio.com" ) )
        handleRdioUrls( urls );
    else if( urls.contains( "soundcloud" ) )
        handleSoundcloudUrls( urls );
    else if( urls.contains( "ex.fm" ) )
        handleExfmUrls( urls );
#ifdef QCA2_FOUND
    else if ( urls.contains( "grooveshark.com" ) )
        handleGroovesharkUrls( urls );
#endif
    else
        handleTrackUrls ( urls );
}


void
DropJob::handleTrackUrls( const QString& urls )
{
    if ( urls.contains( "itunes.apple.com") )
    {
        QStringList tracks = urls.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );

        tDebug() << "Got a list of itunes urls!" << tracks;
        ItunesParser* itunes = new ItunesParser( tracks, this );
        connect( itunes, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;
    }
    else if ( urls.contains( "open.spotify.com/track") || urls.contains( "spotify:track" ) )
    {
        QStringList tracks = urls.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );

        tDebug() << "Got a list of spotify urls!" << tracks;
        SpotifyParser* spot = new SpotifyParser( tracks, this );
        connect( spot, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;
    }
    else if ( urls.contains( "soundcloud" ) )
    {
        QStringList tracks = urls.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );

        tDebug() << "Got a list of Soundcloud tracks!" << tracks;
        SoundcloudParser* sc = new SoundcloudParser( tracks, false, this );
        connect( sc, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;
    }
    else if ( urls.contains( "ex.fm" ) )
    {
        QStringList tracks = urls.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );

        tDebug() << "Got a list of Exfm tracks!" << tracks;
        ExfmParser* exfm = new ExfmParser( tracks, false, this );
        connect( exfm, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;
    }
    else if ( urls.contains( "rdio.com" ) )
    {
        QStringList tracks = urls.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );

        tDebug() << "Got a list of rdio urls!" << tracks;
        RdioParser* rdio = new RdioParser( this );
        connect( rdio, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( onTracksAdded( QList< Tomahawk::query_ptr > ) ) );
        m_queryCount++;

        rdio->parse( tracks );
    }
    else if ( ShortenedLinkParser::handlesUrl( urls ) )
    {
        QStringList tracks = urls.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );

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
    handleAllUrls( urls.join( "\n" ) );
}


void
DropJob::onTracksAdded( const QList<Tomahawk::query_ptr>& tracksList )
{
    tDebug() << Q_FUNC_INFO << tracksList.count();
    
#ifndef ENABLE_HEADLESS
/*    if ( results.isEmpty() )
    {

        const QString which = album.isEmpty() ? "artist" : "album";
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "No tracks found for given %1" ).arg( which ), 5 ) );
    }*/
#endif

    if ( !m_dropJob.isEmpty() )
    {
        m_dropJob.takeFirst()->setFinished();
    }

    m_resultList.append( tracksList );

    if ( --m_queryCount == 0 )
    {
/*        if ( m_onlyLocal )
            removeRemoteSources();

        if ( !m_allowDuplicates )
            removeDuplicates();*/

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
        Q_ASSERT( !item.isNull() );
        if ( item.isNull() )
        {
            m_resultList.removeOne( item );
            continue;
        }
        
        foreach( const Tomahawk::query_ptr &tmpItem, list )
        {
            if ( tmpItem.isNull() )
            {
                list.removeOne( tmpItem );
                continue;
            }
            
            if ( item->album() == tmpItem->album()
                 && item->artist() == tmpItem->artist()
                 && item->track() == tmpItem->track() )
            {
                if ( item->playable() && !tmpItem->playable() )
                    list.replace( list.indexOf( tmpItem ), item );

                contains = true;
                break;
            }
        }
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
        Q_ASSERT( !item.isNull() );
        if ( item.isNull() )
        {
            m_resultList.removeOne( item );
            continue;
        }
        
        bool hasLocalSource = false;
        foreach ( const Tomahawk::result_ptr& result, item->results() )
        {
            if ( !result->collection().isNull() && !result->collection()->source().isNull() &&
                 !result->collection()->source().isNull() && result->collection()->source()->isLocal() )
                hasLocalSource = true;
        }
        if ( hasLocalSource )
            list.append( item );
    }
    m_resultList = list;
}


QList< query_ptr >
DropJob::getArtist( const QString &artist, Tomahawk::ModelMode mode )
{
    Q_UNUSED( mode );
    artist_ptr artistPtr = Artist::get( artist );
    if ( artistPtr->playlistInterface( Mixed )->tracks().isEmpty() )
    {
        m_artistsToKeep.insert( artistPtr );

        connect( artistPtr.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                     SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

#ifndef ENABLE_HEADLESS
        m_dropJob << new DropJobNotifier( QPixmap( RESPATH "images/album-icon.png" ), Album );
        JobStatusView::instance()->model()->addJob( m_dropJob.last() );
#endif

        m_queryCount++;
    }

    return artistPtr->playlistInterface( Mixed )->tracks();
}


QList< query_ptr >
DropJob::getAlbum( const QString& artist, const QString& album )
{
    artist_ptr artistPtr = Artist::get( artist );
    album_ptr albumPtr = Album::get( artistPtr, album );

    if ( albumPtr.isNull() )
        return QList< query_ptr >();

    //FIXME: should check tracksLoaded()
    if ( albumPtr->playlistInterface( Mixed )->tracks().isEmpty() )
    {
        // For albums that don't exist until this moment, we are the main shared pointer holding on.
        // fetching the tracks is asynchronous, so the resulting signal is queued. when we go out of scope we delete
        // the artist_ptr which means we never get the signal delivered. so we hold on to the album pointer till we're done
        m_albumsToKeep.insert( albumPtr );

        connect( albumPtr.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                    SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

#ifndef ENABLE_HEADLESS
        m_dropJob << new DropJobNotifier( QPixmap( RESPATH "images/album-icon.png" ), Album );
        JobStatusView::instance()->model()->addJob( m_dropJob.last() );
#endif

        m_queryCount++;
    }

    return albumPtr->playlistInterface( Mixed )->tracks();
}


QList< query_ptr >
DropJob::getTopTen( const QString& artist )
{
    return getArtist( artist, Tomahawk::InfoSystemMode );
}
