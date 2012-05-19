/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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
#include "Collection.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_AllAlbums.h"
#include "database/DatabaseCommand_TrackStats.h"
#include "Source.h"

#include "utils/Logger.h"

using namespace Tomahawk;


Artist::~Artist()
{
    m_ownRef.clear();

#ifndef ENABLE_HEADLESS
    delete m_cover;
#endif
}


artist_ptr
Artist::get( const QString& name, bool autoCreate )
{
    if ( !Database::instance() || !Database::instance()->impl() )
        return artist_ptr();

    int artid = Database::instance()->impl()->artistId( name, autoCreate );
    if ( artid < 1 && autoCreate )
        return artist_ptr();

    return Artist::get( artid, name );
}


artist_ptr
Artist::get( unsigned int id, const QString& name )
{
    static QHash< unsigned int, artist_ptr > s_artists;
    static QMutex s_mutex;

    QMutexLocker lock( &s_mutex );
    if ( s_artists.contains( id ) )
    {
        return s_artists.value( id );
    }

    artist_ptr a = artist_ptr( new Artist( id, name ), &QObject::deleteLater );
    a->setWeakRef( a.toWeakRef() );

    if ( id > 0 )
        s_artists.insert( id, a );

    return a;
}


Artist::Artist( unsigned int id, const QString& name )
    : QObject()
    , m_id( id )
    , m_name( name )
    , m_infoLoaded( false )
    , m_infoLoading( false )
    , m_simArtistsLoaded( false )
    , m_infoJobs( 0 )
#ifndef ENABLE_HEADLESS
    , m_cover( 0 )
#endif
{
    m_sortname = DatabaseImpl::sortname( name, true );
}


void
Artist::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    Tomahawk::ArtistPlaylistInterface* api = dynamic_cast< Tomahawk::ArtistPlaylistInterface* >( playlistInterface().data() );
    if ( api )
        api->addQueries( tracks );

    emit tracksAdded( tracks );
}


QList<album_ptr>
Artist::albums( ModelMode mode, const Tomahawk::collection_ptr& collection ) const
{
    artist_ptr artist = m_ownRef.toStrongRef();

    bool dbLoaded = m_albumsLoaded.value( DatabaseMode );
    const bool infoLoaded = m_albumsLoaded.value( InfoSystemMode );
    if ( !collection.isNull() )
        dbLoaded = false;

    m_uuid = uuid();
    tDebug() << Q_FUNC_INFO << mode;

    if ( ( mode == DatabaseMode || mode == Mixed ) && !dbLoaded )
    {
        DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( collection, artist );
        cmd->setData( QVariant( collection.isNull() ) );

        connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                        SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, QVariant ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }

    if ( ( mode == InfoSystemMode || mode == Mixed ) && !infoLoaded )
    {
        Tomahawk::InfoSystem::InfoStringHash artistInfo;
        artistInfo["artist"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = m_uuid;
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
        requestData.caller = m_uuid;
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
Artist::onAlbumsFound( const QList< album_ptr >& albums, const QVariant& data )
{
    if ( data.toBool() )
    {
        m_databaseAlbums << albums;
        m_albumsLoaded.insert( DatabaseMode, true );
    }

    emit albumsAdded( albums, DatabaseMode );
}


void
Artist::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != m_uuid )
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
                tDebug() << Q_FUNC_INFO << albumName;
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
            if ( !output.isNull() && output.isValid() )
            {
                const QByteArray ba = returnedData["imgbytes"].toByteArray();
                if ( ba.length() )
                {
                    m_coverBuffer = ba;
                    m_infoLoaded = true;
                    emit coverChanged();
                }
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

        default:
            Q_ASSERT( false );
    }
}


void
Artist::infoSystemFinished( QString target )
{
    Q_UNUSED( target );

    if ( target != m_uuid )
        return;

    if ( --m_infoJobs == 0 )
    {
        disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                    this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

        disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ),
                    this, SLOT( infoSystemFinished( QString ) ) );
    }

    emit updated();
}


#ifndef ENABLE_HEADLESS
QPixmap
Artist::cover( const QSize& size, bool forceLoad ) const
{
    if ( !m_infoLoaded && !m_infoLoading )
    {
        if ( !forceLoad )
            return QPixmap();
        m_uuid = uuid();

        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["artist"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = m_uuid;
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

        m_infoLoading = true;
    }

    if ( !m_cover && !m_coverBuffer.isEmpty() )
    {
        m_cover = new QPixmap();
        m_cover->loadFromData( m_coverBuffer );
        m_coverBuffer.clear();
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
Artist::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::ArtistPlaylistInterface( this ) );
    }

    return m_playlistInterface;
}
