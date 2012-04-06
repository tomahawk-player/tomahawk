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

#include "artist.h"

#include "artistplaylistinterface.h"
#include "collection.h"
#include "database/database.h"
#include "database/databaseimpl.h"
#include "query.h"

#include "utils/logger.h"

using namespace Tomahawk;


Artist::~Artist()
{
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


#ifndef ENABLE_HEADLESS
QPixmap
Artist::cover( const QSize& size, bool forceLoad ) const
{
    if ( !m_infoLoaded || !m_infoLoading )
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
                SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( finished( QString ) ),
                SLOT( infoSystemFinished( QString ) ) );

        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

        m_infoLoading = true;
    }

    if ( !m_cover && !m_coverBuffer.isEmpty() )
    {
        m_cover = new QPixmap();
        m_cover->loadFromData( m_coverBuffer );
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


void
Artist::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != m_uuid ||
         requestData.type != Tomahawk::InfoSystem::InfoArtistImages )
    {
        return;
    }

    if ( !output.isNull() && output.isValid() )
    {
        QVariantMap returnedData = output.value< QVariantMap >();
        const QByteArray ba = returnedData["imgbytes"].toByteArray();
        if ( ba.length() )
        {
            m_coverBuffer = ba;
            emit coverChanged();
        }
    }
}


void
Artist::infoSystemFinished( QString target )
{
    Q_UNUSED( target );

    if ( target != m_uuid )
        return;

    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ),
                this, SLOT( infoSystemFinished( QString ) ) );

    m_infoLoaded = true;
    emit updated();
}


Tomahawk::playlistinterface_ptr
Artist::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::ArtistPlaylistInterface( this ) );
    }

    return m_playlistInterface;
}
