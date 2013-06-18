/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Api_v2_0.h"

#include "audio/AudioEngine.h"
#include "utils/Logger.h"

#include "Api_v2.h"
#include "Query.h"

#include <QxtWeb/QxtWebPageEvent>
#include <QxtWeb/QxtWebSlotService>

#define JSON_ERROR( event, message ) { tLog( LOGVERBOSE ) << Q_FUNC_INFO << message; m_service->sendJsonError( event, message ); }

Api_v2_0::Api_v2_0( Api_v2* parent )
    : QObject( parent )
    , m_service( parent )
{
}

void
Api_v2_0::ping( QxtWebRequestEvent* event )
{
    QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, "pong" );
    e->contentType = "text/plain";
    m_service->postEvent( e );
}

void
Api_v2_0::playback( QxtWebRequestEvent* event, const QString& command )
{
    if ( command == "next ")
    {
        if ( QMetaObject::invokeMethod( AudioEngine::instance(), "next", Qt::QueuedConnection ) )
        {
            m_service->sendJsonOk( event );
        }
        else
        {
            JSON_ERROR( event, "Skipping to the next track failed." );
        }
    }
    else if ( command == "previous" )
    {
        if ( QMetaObject::invokeMethod( AudioEngine::instance(), "previous", Qt::QueuedConnection ) )
        {
            m_service->sendJsonOk( event );
        }
        else
        {
            JSON_ERROR( event, "Rewinding to the previous track failed." );
        }
    }
    else if ( command == "playpause" )
    {
        if ( QMetaObject::invokeMethod( AudioEngine::instance(), "playpause", Qt::QueuedConnection ) )
        {
            m_service->sendJsonOk( event );
        }
        else
        {
            JSON_ERROR( event, "Play/Pause failed." );
        }
    }
    else if ( command == "play" )
    {
        if ( QMetaObject::invokeMethod( AudioEngine::instance(), "play", Qt::QueuedConnection ) )
        {
            m_service->sendJsonOk( event );
        }
        else
        {
            JSON_ERROR( event, "Skipping to the next track failed." );
        }
    }
    else if ( command == "pause" )
    {
        if ( QMetaObject::invokeMethod( AudioEngine::instance(), "pause", Qt::QueuedConnection ) )
        {
            m_service->sendJsonOk( event );
        }
        else
        {
            JSON_ERROR( event, "Skipping to the next track failed." );
        }
    }
    else if ( command == "stop" )
    {
        if ( QMetaObject::invokeMethod( AudioEngine::instance(), "stop", Qt::QueuedConnection ) )
        {
            m_service->sendJsonOk( event );
        }
        else
        {
            JSON_ERROR( event, "Skipping to the next track failed." );
        }
    }
    else if ( command == "lowervolume" )
    {
        if ( QMetaObject::invokeMethod( AudioEngine::instance(), "lowerVolume", Qt::QueuedConnection ) )
        {
            m_service->sendJsonOk( event );
        }
        else
        {
            JSON_ERROR( event, "Skipping to the next track failed." );
        }
    }
    else if ( command == "raisevolume" )
    {
        if ( QMetaObject::invokeMethod( AudioEngine::instance(), "raiseVolume", Qt::QueuedConnection ) )
        {
            m_service->sendJsonOk( event );
        }
        else
        {
            JSON_ERROR( event, "Skipping to the next track failed." );
        }
    }
}
