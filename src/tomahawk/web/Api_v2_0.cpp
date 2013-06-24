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
#include "jobview/JobStatusModel.h"
#include "jobview/JobStatusView.h"
#include "utils/Logger.h"
#include "web/apiv2/ACLJobStatusItem.h"

#include "Api_v2.h"
#include "Query.h"

#include <QSslKey>
#include <QxtWeb/QxtWebPageEvent>
#include <QxtWeb/QxtWebSlotService>

// Assumptions: QxtWebRequestEvent instance is called event and result is true on success
#define JSON_REPLY( result, message ) jsonReply( event, Q_FUNC_INFO, message, !result )

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
    if ( !checkAuthentication( event ) )
    {
        return;
    }
    if ( command == "next")
    {
        JSON_REPLY( QMetaObject::invokeMethod( AudioEngine::instance(), "next", Qt::QueuedConnection ) , "Skipping to the next track failed." );
    }
    else if ( command == "previous" )
    {
        JSON_REPLY( QMetaObject::invokeMethod( AudioEngine::instance(), "previous", Qt::QueuedConnection ), "Rewinding to the previous track failed." );
    }
    else if ( command == "playpause" )
    {
        JSON_REPLY( QMetaObject::invokeMethod( AudioEngine::instance(), "playpause", Qt::QueuedConnection ), "Play/Pause failed." );
    }
    else if ( command == "play" )
    {
        JSON_REPLY( QMetaObject::invokeMethod( AudioEngine::instance(), "play", Qt::QueuedConnection ), "Starting the playback failed." );
    }
    else if ( command == "pause" )
    {
        JSON_REPLY( QMetaObject::invokeMethod( AudioEngine::instance(), "pause", Qt::QueuedConnection ), "Pausing the current track failed." );
    }
    else if ( command == "stop" )
    {
        JSON_REPLY( QMetaObject::invokeMethod( AudioEngine::instance(), "stop", Qt::QueuedConnection ), "Stopping the current track failed." );
    }
    else if ( command == "lowervolume" )
    {
        JSON_REPLY( QMetaObject::invokeMethod( AudioEngine::instance(), "lowerVolume", Qt::QueuedConnection ), "Lowering volume failed." );
    }
    else if ( command == "raisevolume" )
    {
        JSON_REPLY( QMetaObject::invokeMethod( AudioEngine::instance(), "raiseVolume", Qt::QueuedConnection ), "Raising volume failed." );
    }
    else
    {
        m_service->sendJsonError( event, "No such playback command." );
    }
}


void
Api_v2_0::requestauth( QxtWebRequestEvent *event )
{
    tLog() << Q_FUNC_INFO;
    if ( checkAuthentication( event ) )
    {
        // A decision was already made.
        m_service->sendJsonOk( event );
        return;
    }

    if ( event->isSecure && !event->clientCertificate.isNull() )
    {
        tLog() << Q_FUNC_INFO;
        QSslCertificate certificate = event->clientCertificate;
        QString clientName = certificate.subjectInfo( QSslCertificate::CommonName );

        if ( m_users.contains( clientName ) && m_users.value( clientName )->aclDecision() == Api2User::Deny )
        {
            // Access was already denied
            jsonUnauthenticated( event );
            return;
        }

        QSharedPointer<Api2User> user( new Api2User( clientName ) );
        user->setClientDescription( certificate.subjectInfo( QSslCertificate::OrganizationalUnitName ) );
        user->setClientName( certificate.subjectInfo( QSslCertificate::Organization ) );
        user->setPubkey( certificate.publicKey() );
        // TODO: Do not readd users
        m_users[ clientName ] = user;
        // TODO: ACL decision
        Tomahawk::APIv2::ACLJobStatusItem* job = new Tomahawk::APIv2::ACLJobStatusItem( user );
        JobStatusView::instance()->model()->addJob( job );
    }
    else
    {
        // TODO
    }
}


void
Api_v2_0::jsonReply( QxtWebRequestEvent* event, const char* funcInfo, const QString& errorMessage, bool isError )
{
    if ( isError )
    {
        tLog( LOGVERBOSE ) << funcInfo << errorMessage;
        m_service->sendJsonError( event, errorMessage );
    }
    else
    {
        m_service->sendJsonOk( event );

    }
}


void
Api_v2_0::jsonUnauthenticated( QxtWebRequestEvent *event )
{
    QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, "{ result: \"error\", error: \"Method call needs to be authenticated.\" }" );
    e->contentType = "application/json";
    e->status = 401;
    e->statusMessage = "Method call needs to be authenticated.";
    m_service->postEvent( e );
}


bool
Api_v2_0::checkAuthentication( QxtWebRequestEvent* event )
{
    if ( event->isSecure && !event->clientCertificate.isNull() )
    {
        // Using SSL Certificate authentication
        QString clientName = event->clientCertificate.subjectInfo( QSslCertificate::CommonName );
        QSslKey pubkey = event->clientCertificate.publicKey();
        if ( m_users.contains( clientName ) )
        {
            const QSharedPointer<Api2User> user = m_users.value( clientName );
            if ( user->aclDecision() == Api2User::FullAccess && user->pubkey() == pubkey )
            {
                return true;
            }
        }
    }
    // TODO: Auth!
    // * Shared secret between two clients when talking via SSL
    // * sth else when connecting without httpS
    // * a more secure version of digest auth
    return false;
}
