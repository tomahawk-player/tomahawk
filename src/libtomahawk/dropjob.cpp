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

#include "utils/spotifyparser.h"
#include "utils/rdioparser.h"
#include "utils/shortenedlinkparser.h"
#include "utils/logger.h"
#include "globalactionmanager.h"

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
DropJob::acceptsMimeData( const QMimeData* data, bool tracksOnly )
{
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

    // crude check for spotify tracks
    if ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "spotify" ) &&
       ( tracksOnly ? data->data( "text/plain" ).contains( "track" ) : true ) )
        return true;

    // crude check for rdio tracks
    if ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "rdio.com" ) &&
        ( tracksOnly ? data->data( "text/plain" ).contains( "track" ) : true ) )
        return true;

    // We whitelist t.co and bit.ly (and j.mp) since they do some link checking. Often playable (e.g. spotify..) links hide behind them,
    //  so we do an extra level of lookup
    if ( ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "bit.ly" ) ) ||
         ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "j.mp" ) ) ||
         ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "t.co" ) ) ||
         ( data->hasFormat( "text/plain" ) && data->data( "text/plain" ).contains( "rd.io" ) ) )
        return true;

    return false;
}


void
DropJob::tracksFromMimeData( const QMimeData* data, bool allowDuplicates )
{
    m_allowDuplicates = allowDuplicates;

    parseMimeData( data );

    if ( m_queryCount == 0 )
    {
        if ( !allowDuplicates )
            removeDuplicates();

        emit tracks( m_resultList );
        deleteLater();
    }
}

void
DropJob::parseMimeData( const QMimeData *data )
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
    else if ( data->hasFormat( "text/plain" ) )
    {
        QString plainData = QString::fromUtf8( data->data( "text/plain" ).constData() );
        tDebug() << "Got text/plain mime data:" << data->data( "text/plain" ) << "decoded to:" << plainData;
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
            queries << *query;
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
            q->addResults( QList< result_ptr >() << *result );
            queries << q;
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

        artist_ptr artistPtr = Artist::get( artist );
        album_ptr albumPtr = Album::get( artistPtr, album );
        if ( albumPtr->tracks().isEmpty() )
        {
            connect( albumPtr.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                                     SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );
            m_queryCount++;
        }
        else
            queries << albumPtr->tracks();
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

        artist_ptr artistPtr = Artist::get( artist );
        if ( artistPtr->tracks().isEmpty() )
        {
            connect( artistPtr.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                                     SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );
            m_queryCount++;
        }
        else
            queries << artistPtr->tracks();
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
DropJob::handleTrackUrls( const QString& urls )
{
    if ( urls.contains( "open.spotify.com/track") ||
         urls.contains( "spotify:track" ) )
    {
        QStringList tracks = urls.split( "\n" );

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
        rdio->parse( tracks );
        m_queryCount++;
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
