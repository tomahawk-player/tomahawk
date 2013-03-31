/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "Artist.h"

#include "ArtistPlaylistInterface.h"
#include "collection/Collection.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_AllAlbums.h"
#include "database/DatabaseCommand_TrackStats.h"
#include "database/IdThreadWorker.h"
#include "Source.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QReadWriteLock>

using namespace Tomahawk;

QHash< QString, artist_wptr > Artist::s_artistsByName = QHash< QString, artist_wptr >();
QHash< unsigned int, artist_wptr > Artist::s_artistsById = QHash< unsigned int, artist_wptr >();

static QMutex s_nameCacheMutex;
static QReadWriteLock s_idMutex;

Artist::~Artist()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Deleting artist:" << m_name;
    m_ownRef.clear();

#ifndef ENABLE_HEADLESS
    delete m_cover;
#endif
}


artist_ptr
Artist::get( const QString& name, bool autoCreate )
{
    if ( name.isEmpty() )
        return artist_ptr();

    QMutexLocker lock( &s_nameCacheMutex );
    const QString key = name.toLower();
    if ( s_artistsByName.contains( key ) )
    {
        artist_wptr artist = s_artistsByName.value( key );
        if ( !artist.isNull() )
            return artist.toStrongRef();
    }

    if ( !Database::instance() || !Database::instance()->impl() )
        return artist_ptr();

    artist_ptr artist = artist_ptr( new Artist( name ), &Artist::deleteLater );
    artist->setWeakRef( artist.toWeakRef() );
    artist->loadId( autoCreate );
    s_artistsByName.insert( key, artist );

    return artist;
}


artist_ptr
Artist::get( unsigned int id, const QString& name )
{
    s_idMutex.lockForRead();
    if ( s_artistsById.contains( id ) )
    {
        artist_wptr artist = s_artistsById.value( id );
        s_idMutex.unlock();

        if ( !artist.isNull() )
            return artist;
    }
    s_idMutex.unlock();

    QMutexLocker lock( &s_nameCacheMutex );
    const QString key = name.toLower();
    if ( s_artistsByName.contains( key ) )
    {
        artist_wptr artist = s_artistsByName.value( key );
        if ( !artist.isNull() )
            return artist;
    }

    artist_ptr a = artist_ptr( new Artist( id, name ), &Artist::deleteLater );
    a->setWeakRef( a.toWeakRef() );
    s_artistsByName.insert( key, a );

    if ( id > 0 )
    {
        s_idMutex.lockForWrite();
        s_artistsById.insert( id, a );
        s_idMutex.unlock();
    }

    return a;
}


Artist::Artist( unsigned int id, const QString& name )
    : QObject()
    , m_waitingForFuture( false )
    , m_id( id )
    , m_name( name )
    , m_coverLoaded( false )
    , m_coverLoading( false )
    , m_simArtistsLoaded( false )
    , m_biographyLoaded( false )
    , m_infoJobs( 0 )
#ifndef ENABLE_HEADLESS
    , m_cover( 0 )
#endif
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Creating artist:" << id << name;
    m_sortname = DatabaseImpl::sortname( name, true );
}


Artist::Artist( const QString& name )
    : QObject()
    , m_waitingForFuture( true )
    , m_id( 0 )
    , m_name( name )
    , m_coverLoaded( false )
    , m_coverLoading( false )
    , m_simArtistsLoaded( false )
    , m_biographyLoaded( false )
    , m_infoJobs( 0 )
#ifndef ENABLE_HEADLESS
    , m_cover( 0 )
#endif
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Creating artist:" << name;
    m_sortname = DatabaseImpl::sortname( name, true );
}


void
Artist::deleteLater()
{
    QMutexLocker lock( &s_nameCacheMutex );

    const QString key = m_name.toLower();
    if ( s_artistsByName.contains( key ) )
    {
        s_artistsByName.remove( key );
    }

    if ( m_id > 0 )
    {
        s_idMutex.lockForWrite();
        if ( s_artistsById.contains( m_id ) )
        {
            s_artistsById.remove( m_id );
        }
        s_idMutex.unlock();
    }

    QObject::deleteLater();
}


void
Artist::onTracksLoaded( Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection )
{
    emit tracksAdded( playlistInterface( mode, collection )->tracks(), mode, collection );
}


QList<album_ptr>
Artist::albums( ModelMode mode, const Tomahawk::collection_ptr& collection ) const
{
    artist_ptr artist = m_ownRef.toStrongRef();

    bool dbLoaded = m_albumsLoaded.value( DatabaseMode );
    const bool infoLoaded = m_albumsLoaded.value( InfoSystemMode );
    if ( !collection.isNull() )
        dbLoaded = false;

    if ( ( mode == DatabaseMode || mode == Mixed ) && !dbLoaded )
    {
        if ( collection.isNull() )
        {
            DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( collection, artist );
            cmd->setData( QVariant( collection.isNull() ) );

            connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                          SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, QVariant ) ) );

            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
        }
        else
        {
            //collection is *surely* not null, and might be a ScriptCollection
            Tomahawk::AlbumsRequest* cmd = collection->requestAlbums( artist );

            // There is also a signal albums( QList, QVariant ).
            // The QVariant might carry a bool that says whether the dbcmd was executed for a null collection
            // but here we know for a fact that the collection is not null, so we'll happily ignore it
            connect( dynamic_cast< QObject* >( cmd ), SIGNAL( albums( QList<Tomahawk::album_ptr> ) ),
                     this, SLOT( onAlbumsFound( QList<Tomahawk::album_ptr> ) ) );

            cmd->enqueue();
        }
    }

    if ( ( mode == InfoSystemMode || mode == Mixed ) && !infoLoaded )
    {
        Tomahawk::InfoSystem::InfoStringHash artistInfo;
        artistInfo["artist"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = infoid();
        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
        requestData.type = Tomahawk::InfoSystem::InfoArtistReleases;

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                 SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                 SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                 SIGNAL( finished( QString ) ),
                 SLOT( infoSystemFinished( QString ) ), Qt::UniqueConnection );

        m_infoJobs++;
        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }

    if ( !collection.isNull() )
        return QList<album_ptr>();

    switch ( mode )
    {
        case DatabaseMode:
            return m_databaseAlbums;
        case InfoSystemMode:
            return m_officialAlbums;
        default:
            return m_databaseAlbums + m_officialAlbums;
    }
}


QList<Tomahawk::artist_ptr>
Artist::similarArtists() const
{
    if ( !m_simArtistsLoaded )
    {
        Tomahawk::InfoSystem::InfoStringHash artistInfo;
        artistInfo["artist"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = infoid();
        requestData.customData = QVariantMap();

        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
        requestData.type = Tomahawk::InfoSystem::InfoArtistSimilars;
        requestData.requestId = TomahawkUtils::infosystemRequestId();

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( finished( QString ) ),
                SLOT( infoSystemFinished( QString ) ), Qt::UniqueConnection );

        m_infoJobs++;
        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }

    return m_similarArtists;
}


void
Artist::loadId( bool autoCreate )
{
    Q_ASSERT( m_waitingForFuture );

    IdThreadWorker::getArtistId( m_ownRef.toStrongRef(), autoCreate );
}


void
Artist::setIdFuture( QFuture<unsigned int> future )
{
    m_idFuture = future;
}


unsigned int
Artist::id() const
{
    s_idMutex.lockForRead();
    const bool waiting = m_waitingForFuture;
    unsigned int finalid = m_id;
    s_idMutex.unlock();

    if ( waiting )
    {
//        qDebug() << Q_FUNC_INFO << "Asked for artist ID and NOT loaded yet" << m_name << m_idFuture.isFinished();
        m_idFuture.waitForFinished();
//        qDebug() << "DONE WAITING:" << m_idFuture.resultCount() << m_idFuture.isResultReadyAt( 0 ) << m_idFuture.isCanceled() << m_idFuture.isFinished() << m_idFuture.isPaused() << m_idFuture.isRunning() << m_idFuture.isStarted();
        finalid = m_idFuture.result();

//        qDebug() << Q_FUNC_INFO << "Got loaded artist:" << m_name << finalid;

        s_idMutex.lockForWrite();
        m_id = finalid;
        m_waitingForFuture = false;

        if ( m_id > 0 )
            s_artistsById.insert( m_id, m_ownRef.toStrongRef() );

        s_idMutex.unlock();
    }

    return m_id;
}


QString
Artist::biography() const
{
    if ( !m_biographyLoaded )
    {
        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["artist"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = infoid();
        requestData.type = Tomahawk::InfoSystem::InfoArtistBiography;
        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
        requestData.customData = QVariantMap();

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( finished( QString ) ),
                SLOT( infoSystemFinished( QString ) ), Qt::UniqueConnection );

        m_infoJobs++;
        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }

    return m_biography;
}


void
Artist::loadStats()
{
    artist_ptr a = m_ownRef.toStrongRef();

    DatabaseCommand_TrackStats* cmd = new DatabaseCommand_TrackStats( a );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


QList< Tomahawk::PlaybackLog >
Artist::playbackHistory( const Tomahawk::source_ptr& source ) const
{
    QList< Tomahawk::PlaybackLog > history;

    foreach ( const PlaybackLog& log, m_playbackHistory )
    {
        if ( source.isNull() || log.source == source )
        {
            history << log;
        }
    }

    return history;
}


void
Artist::setPlaybackHistory( const QList< Tomahawk::PlaybackLog >& playbackData )
{
    m_playbackHistory = playbackData;
    emit statsLoaded();
}


unsigned int
Artist::playbackCount( const source_ptr& source )
{
    unsigned int count = 0;
    foreach ( const PlaybackLog& log, m_playbackHistory )
    {
        if ( source.isNull() || log.source == source )
            count++;
    }

    return count;
}


void
Artist::onAlbumsFound( const QList< album_ptr >& albums, const QVariant& collectionIsNull )
{
    if ( collectionIsNull.toBool() )
    {
        m_databaseAlbums << albums;
        m_albumsLoaded.insert( DatabaseMode, true );
    }

    emit albumsAdded( albums, DatabaseMode );
}


void
Artist::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != infoid() )
        return;

    QVariantMap returnedData = output.value< QVariantMap >();
    switch ( requestData.type )
    {
        case Tomahawk::InfoSystem::InfoArtistReleases:
        {
            QStringList albumNames = returnedData[ "albums" ].toStringList();
            Tomahawk::InfoSystem::InfoStringHash inputInfo;
            inputInfo = requestData.input.value< InfoSystem::InfoStringHash >();

            QList< album_ptr > albums;
            foreach ( const QString& albumName, albumNames )
            {
                Tomahawk::album_ptr album = Tomahawk::Album::get( m_ownRef.toStrongRef(), albumName, false );
                m_officialAlbums << album;
                albums << album;
            }

            m_albumsLoaded.insert( InfoSystemMode, true );
            if ( m_officialAlbums.count() )
                emit albumsAdded( albums, InfoSystemMode );

            break;
        }

        case Tomahawk::InfoSystem::InfoArtistImages:
        {
            if ( output.isNull() )
            {
                m_coverLoaded = true;
            }
            else if ( output.isValid() )
            {
                const QByteArray ba = returnedData["imgbytes"].toByteArray();
                if ( ba.length() )
                {
                    m_coverBuffer = ba;
                }

                m_coverLoaded = true;
                emit coverChanged();
            }

            break;
        }

        case InfoSystem::InfoArtistSimilars:
        {
            const QStringList artists = returnedData["artists"].toStringList();
            foreach ( const QString& artist, artists )
            {
                m_similarArtists << Artist::get( artist );
            }

            m_simArtistsLoaded = true;
            emit similarArtistsLoaded();

            break;
        }

        case InfoSystem::InfoArtistBiography:
        {
            QVariantMap bmap = output.toMap();
            foreach ( const QString& source, bmap.keys() )
            {
                if ( source == "last.fm" )
                    m_biography = bmap[ source ].toHash()[ "text" ].toString();
            }

            m_biographyLoaded = true;
            emit biographyLoaded();

            break;
        }

        default:
            Q_ASSERT( false );
    }
}


void
Artist::infoSystemFinished( QString target )
{
    Q_UNUSED( target );

    if ( target != infoid() )
        return;

    if ( --m_infoJobs == 0 )
    {
        disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                    this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

        disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ),
                    this, SLOT( infoSystemFinished( QString ) ) );
    }

    m_coverLoading = false;

    emit updated();
}


#ifndef ENABLE_HEADLESS
QPixmap
Artist::cover( const QSize& size, bool forceLoad ) const
{
    if ( !m_coverLoaded && !m_coverLoading )
    {
        if ( !forceLoad )
            return QPixmap();

        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["artist"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = infoid();
        requestData.type = Tomahawk::InfoSystem::InfoArtistImages;
        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
        requestData.customData = QVariantMap();

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( finished( QString ) ),
                SLOT( infoSystemFinished( QString ) ), Qt::UniqueConnection );

        m_infoJobs++;
        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

        m_coverLoading = true;
    }

    if ( !m_cover && !m_coverBuffer.isEmpty() )
    {
        QPixmap cover;
        cover.loadFromData( m_coverBuffer );
        m_coverBuffer.clear();

        m_cover = new QPixmap( TomahawkUtils::squareCenterPixmap( cover ) );
    }

    if ( m_cover && !m_cover->isNull() && !size.isEmpty() )
    {
        if ( m_coverCache.contains( size.width() ) )
        {
            return m_coverCache.value( size.width() );
        }

        QPixmap scaledCover;
        scaledCover = m_cover->scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        m_coverCache.insert( size.width(), scaledCover );
        return scaledCover;
    }

    if ( m_cover )
        return *m_cover;
    else
        return QPixmap();
}
#endif


Tomahawk::playlistinterface_ptr
Artist::playlistInterface( ModelMode mode, const Tomahawk::collection_ptr& collection )
{
    playlistinterface_ptr pli = m_playlistInterface[ mode ][ collection ];

    if ( pli.isNull() )
    {
        pli = Tomahawk::playlistinterface_ptr( new Tomahawk::ArtistPlaylistInterface( this, mode, collection ) );
        connect( pli.data(), SIGNAL( tracksLoaded( Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                               SLOT( onTracksLoaded( Tomahawk::ModelMode, Tomahawk::collection_ptr ) ) );

        m_playlistInterface[ mode ][ collection ] = pli;
    }

    return pli;
}


QList<Tomahawk::query_ptr>
Artist::tracks( ModelMode mode, const Tomahawk::collection_ptr& collection )
{
    return playlistInterface( mode, collection )->tracks();
}


QString
Artist::infoid() const
{
    if ( m_uuid.isEmpty() )
        m_uuid = uuid();

    return m_uuid;
}
