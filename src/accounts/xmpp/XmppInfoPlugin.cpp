/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Dominik Schmidt <domme@tomahawk-player.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "GlobalActionManager.h"
#include "sip/XmppSip.h"
#include "utils/Logger.h"
#include "TomahawkSettings.h"


// remove now playing status after PAUSE_TIMEOUT seconds
static const int PAUSE_TIMEOUT = 10;

Tomahawk::InfoSystem::XmppInfoPlugin::XmppInfoPlugin( XmppSipPlugin* sipPlugin )
    : m_sipPlugin( sipPlugin )
    , m_pauseTimer( this )
{
    Q_ASSERT( sipPlugin->m_client );

    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;

    m_pauseTimer.setSingleShot( true );
    connect( &m_pauseTimer, SIGNAL( timeout() ),
             this, SLOT( audioStopped() ) );
}


Tomahawk::InfoSystem::XmppInfoPlugin::~XmppInfoPlugin()
{
}


void
Tomahawk::InfoSystem::XmppInfoPlugin::init()
{
    if ( QThread::currentThread() != Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() )
    {
        QMetaObject::invokeMethod( this, "init", Qt::QueuedConnection );
        return;
    }

    if ( m_sipPlugin.isNull() )
        return;

    connect( this, SIGNAL( publishTune( QUrl, Tomahawk::InfoSystem::InfoStringHash ) ), m_sipPlugin.data(), SLOT( publishTune( QUrl, Tomahawk::InfoSystem::InfoStringHash ) ), Qt::QueuedConnection );
}


void
Tomahawk::InfoSystem::XmppInfoPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
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
        emit publishTune( QUrl(), Tomahawk::InfoSystem::InfoStringHash() );
        return;
    }

    if ( !map.contains( "trackinfo" ) || !map[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug() << Q_FUNC_INFO << "did not find an infostringhash";
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash info = map[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();

    QUrl url;
    if ( pushInfoPair.first.contains( "shorturl" ) )
        url = pushInfoPair.first[ "shorturl" ].toUrl();
    else
        url = GlobalActionManager::instance()->openLink( info.value( "title" ), info.value( "artist" ), info.value( "album" ) );

    emit publishTune( url, info );
}


void
Tomahawk::InfoSystem::XmppInfoPlugin::audioPaused()
{
}


void
Tomahawk::InfoSystem::XmppInfoPlugin::audioStopped()
{
    emit publishTune( QUrl(), Tomahawk::InfoSystem::InfoStringHash() );
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
