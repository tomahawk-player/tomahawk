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


#include "TwitterInfoPlugin.h"

#include "accounts/twitter/TwitterAccount.h"

#include "GlobalActionManager.h"
#include "utils/Logger.h"
#include "Source.h"

#include <QTweetLib/qtweetaccountverifycredentials.h>
#include <QTweetLib/qtweetstatusupdate.h>

namespace Tomahawk
{

namespace InfoSystem
{

TwitterInfoPlugin::TwitterInfoPlugin( Tomahawk::Accounts::TwitterAccount* account )
    : m_account( account )
{
    m_supportedPushTypes << InfoShareTrack << InfoLove;
}


void
TwitterInfoPlugin::init()
{
    if ( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread() && thread() != Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() )
    {
        tDebug() << "Failure: move to the worker thread before running init";
        return;
    }

    QVariantHash credentials = m_account->credentials();
    if ( credentials[ "oauthtoken" ].toString().isEmpty() || credentials[ "oauthtokensecret" ].toString().isEmpty() )
    {
        tDebug() << "TwitterInfoPlugin has empty Twitter credentials; not connecting";
        return;
    }

    if ( refreshTwitterAuth() )
    {
        QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( m_twitterAuth.data(), this );
        connect( credVerifier, SIGNAL( parsedUser( const QTweetUser & ) ), SLOT( connectAuthVerifyReply( const QTweetUser & ) ) );
        credVerifier->verify();
    }
}


TwitterInfoPlugin::~TwitterInfoPlugin()
{
    tDebug() << Q_FUNC_INFO;
}


bool
TwitterInfoPlugin::refreshTwitterAuth()
{
    tDebug() << Q_FUNC_INFO << "begin" << this;
    if ( !m_twitterAuth.isNull() )
        delete m_twitterAuth.data();

    Q_ASSERT( TomahawkUtils::nam() != 0 );
    tDebug() << Q_FUNC_INFO << "with nam" << TomahawkUtils::nam() << this;
    m_twitterAuth = QPointer< TomahawkOAuthTwitter >( new TomahawkOAuthTwitter( TomahawkUtils::nam(), this ) );

    if ( m_twitterAuth.isNull() )
      return false;

    m_twitterAuth.data()->setOAuthToken( m_account->credentials()[ "oauthtoken" ].toString().toLatin1() );
    m_twitterAuth.data()->setOAuthTokenSecret( m_account->credentials()[ "oauthtokensecret" ].toString().toLatin1() );

    return true;
}


void
TwitterInfoPlugin::connectAuthVerifyReply( const QTweetUser &user )
{
    if ( user.id() == 0 )
    {
        tDebug() << "TwitterInfoPlugin could not authenticate to Twitter" << this;
        deleteLater();
        return;
    }
    else
    {
        tDebug() << "TwitterInfoPlugin successfully authenticated to Twitter" << this;
        return;
    }
}


void
TwitterInfoPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    tDebug() << Q_FUNC_INFO;
    if ( !isValid() )
    {
        tDebug() << Q_FUNC_INFO << "Plugin not valid, deleting and returning";
        deleteLater();
        return;
    }

    Tomahawk::InfoSystem::PushInfoPair pushInfoPair = pushData.infoPair;

    if ( !pushInfoPair.second.canConvert< QVariantMap >() )
    {
        tLog() << Q_FUNC_INFO << "Failed to find QVariantMap!";
        return;
    }

    QVariantMap map = pushInfoPair.second.toMap();

    if ( !map.contains( "accountlist" ) || !map[ "accountlist" ].canConvert< QStringList >() )
    {
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Cowardly failing to send out a message without an account list present";
        return;
    }

    const QStringList accountList = map[ "accountlist" ].toStringList();
    if ( !accountList.contains( "all" ) && !accountList.contains( m_account->accountId() ) )
    {
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Our account not in the list, not tweeting out";
        return;
    }

    if ( !map.contains( "message" ) && ( !map.contains( "trackinfo" ) || !map[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() ) )
    {
        tLog() << Q_FUNC_INFO << "Failed to find message or trackinfo";
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash info;
    QString msg;
    if ( !map.contains( "message" ) )
    {
        info = map[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();
        msg = tr( "Listening to \"%1\" by %2 and loving it! %3" )
                .arg( info[ "title" ] )
                .arg( info[ "artist" ] )
                .arg( pushInfoPair.first.contains( "shorturl" ) ?
                        pushInfoPair.first[ "shorturl" ].toUrl().toString() :
                        GlobalActionManager::instance()->openLink( info[ "title" ], info[ "artist" ], info[ "album" ] ).toString() );
    }
    else
        msg = map[ "message" ].toString();

    QTweetStatusUpdate *statUpdate = new QTweetStatusUpdate( m_twitterAuth.data(), this );
    connect( statUpdate, SIGNAL( postedStatus(const QTweetStatus &) ), SLOT( postLovedStatusUpdateReply(const QTweetStatus &) ) );
    connect( statUpdate, SIGNAL( error(QTweetNetBase::ErrorCode, const QString&) ), SLOT( postLovedStatusUpdateError(QTweetNetBase::ErrorCode, const QString &) ) );
    tDebug() << Q_FUNC_INFO << "Posting message:" << msg;
    statUpdate->post( msg );
}


void
TwitterInfoPlugin::postLovedStatusUpdateReply( const QTweetStatus& status )
{
    if ( status.id() == 0 )
        tDebug() << Q_FUNC_INFO << "Failed to post loved status";
    else
        tDebug() << Q_FUNC_INFO << "Successfully posted loved status";
}


void
TwitterInfoPlugin::postLovedStatusUpdateError( QTweetNetBase::ErrorCode code, const QString& errorMsg )
{
    tDebug() << Q_FUNC_INFO << "Error posting love message, error code is " << code << ", error message is " << errorMsg;
}


bool
TwitterInfoPlugin::isValid() const
{
    return !m_twitterAuth.isNull();
}

}

}
