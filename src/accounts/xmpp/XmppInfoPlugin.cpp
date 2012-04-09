/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Dominik Schmidt <domme@tomahawk-player.org>
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


#include "XmppInfoPlugin.h"

#include "globalactionmanager.h"
#include "sip/xmppsip.h"
#include "utils/logger.h"

#include <jreen/tune.h>
#include <jreen/pubsubmanager.h>
#include <jreen/jid.h>

#include <jreen/client.h>

// remove now playing status after PAUSE_TIMEOUT seconds
static const int PAUSE_TIMEOUT = 60;

Tomahawk::InfoSystem::XmppInfoPlugin::XmppInfoPlugin(XmppSipPlugin* sipPlugin)
    : m_sipPlugin( sipPlugin )
    , m_pubSubManager( 0 )
    , m_pauseTimer( this )
{
    Q_ASSERT( sipPlugin->m_client );

    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;

    m_pubSubManager = new Jreen::PubSub::Manager( sipPlugin->m_client );
    m_pubSubManager->addEntityType< Jreen::Tune >();

    // Clear status
    Jreen::Tune::Ptr tune( new Jreen::Tune() );
    m_pubSubManager->publishItems(QList<Jreen::Payload::Ptr>() << tune, Jreen::JID());
    
    m_pauseTimer.setSingleShot( true );
    connect( &m_pauseTimer, SIGNAL( timeout() ),
             this, SLOT( audioStopped() ) );
}


Tomahawk::InfoSystem::XmppInfoPlugin::~XmppInfoPlugin()
{
    //Note: the next two lines don't currently work, because the deletion wipes out internally posted events, need to talk to euro about a fix
    Jreen::Tune::Ptr tune( new Jreen::Tune() );
    m_pubSubManager->publishItems(QList<Jreen::Payload::Ptr>() << tune, Jreen::JID());
    m_pubSubManager->deleteLater();
}


void
Tomahawk::InfoSystem::XmppInfoPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    tDebug() << Q_FUNC_INFO << m_sipPlugin->m_client->jid().full();

    if( m_sipPlugin->m_account->configuration().value("publishtracks").toBool() == false )
    {
        tDebug() << Q_FUNC_INFO <<  m_sipPlugin->m_client->jid().full() << "Not publishing now playing info (disabled in account config)";
        return;
    }

    switch ( pushData.type )
    {
        case InfoNowPlaying:
        case InfoNowResumed:
            m_pauseTimer.stop();
            audioStarted( pushData.infoPair );
            break;
        case InfoNowPaused:
            m_pauseTimer.start( PAUSE_TIMEOUT * 1000 );
            audioPaused();
            break;
        case InfoNowStopped:
            m_pauseTimer.stop();
            audioStopped();
            break;

        default:
            return;
    }
}


void
Tomahawk::InfoSystem::XmppInfoPlugin::audioStarted( const Tomahawk::InfoSystem::PushInfoPair &pushInfoPair )
{
    if ( !pushInfoPair.second.canConvert< QVariantMap >() )
    {
        tDebug() << Q_FUNC_INFO << "Failed to convert data to a QVariantMap";
        return;
    }
    
    QVariantMap map = pushInfoPair.second.toMap();
    if ( map.contains( "private" ) && map[ "private" ] == TomahawkSettings::FullyPrivate )
    {
        Jreen::Tune::Ptr tune( new Jreen::Tune() );
        m_pubSubManager->publishItems( QList<Jreen::Payload::Ptr>() << tune, Jreen::JID() );
        return;
    }
        
    if ( !map.contains( "trackinfo" ) || !map[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug() << Q_FUNC_INFO << "did not find an infostringhash";
        return;
    }
    
    Tomahawk::InfoSystem::InfoStringHash info = map[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();
    tDebug() << Q_FUNC_INFO << m_sipPlugin->m_client->jid().full() << info;
    
    Jreen::Tune::Ptr tune( new Jreen::Tune() );

    tune->setTitle( info.value( "title" ) );
    tune->setArtist( info.value( "artist" ) );
    tune->setLength( info.value("duration").toInt() );
    tune->setTrack( info.value("albumpos") );
    if ( pushInfoPair.first.contains( "shorturl" ) )
        tune->setUri( pushInfoPair.first[ "shorturl" ].toUrl() );
    else
        tune->setUri( GlobalActionManager::instance()->openLink( info.value( "title" ), info.value( "artist" ), info.value( "album" ) ) );

    tDebug() << Q_FUNC_INFO << "Setting URI of " << tune->uri().toString();
    //TODO: provide a rating once available in Tomahawk
    tune->setRating( 10 );

    //TODO: it would be nice to set Spotify, Dilandau etc here, but not the jabber ids of friends
    tune->setSource( "Tomahawk" );

    m_pubSubManager->publishItems( QList<Jreen::Payload::Ptr>() << tune, Jreen::JID() );
}

void
Tomahawk::InfoSystem::XmppInfoPlugin::audioPaused()
{
    tDebug() << Q_FUNC_INFO << m_sipPlugin->m_client->jid().full();
}

void
Tomahawk::InfoSystem::XmppInfoPlugin::audioStopped()
{
    tDebug() << Q_FUNC_INFO << m_sipPlugin->m_client->jid().full();

    Jreen::Tune::Ptr tune( new Jreen::Tune() );
    m_pubSubManager->publishItems(QList<Jreen::Payload::Ptr>() << tune, Jreen::JID());
}


void
Tomahawk::InfoSystem::XmppInfoPlugin::getInfo(Tomahawk::InfoSystem::InfoRequestData requestData)
{
    Q_UNUSED( requestData );
}


void
Tomahawk::InfoSystem::XmppInfoPlugin::notInCacheSlot(const Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData)
{
    Q_UNUSED( criteria );
    Q_UNUSED( requestData );
}
