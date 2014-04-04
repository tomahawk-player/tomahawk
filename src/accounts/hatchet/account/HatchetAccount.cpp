/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "HatchetAccount.h"

#include "HatchetAccountConfig.h"
#include "utils/Closure.h"
#include "utils/Logger.h"
#include "sip/HatchetSip.h"
#include "utils/TomahawkUtils.h"
#include "utils/NetworkAccessManager.h"

#include <QHostInfo>
#include <QFile>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QUuid>
#include <QtPlugin>

#include <qjson/parser.h>
#include <qjson/serializer.h>

namespace Tomahawk
{
namespace Accounts
{
namespace Hatchet
{

static QPixmap* s_icon = 0;
HatchetAccount* HatchetAccount::s_instance  = 0;

const QString c_loginServer("https://auth.hatchet.is/v1");
const QString c_accessTokenServer("https://auth.hatchet.is/v1");

HatchetAccountFactory::HatchetAccountFactory()
{
#ifndef ENABLE_HEADLESS
    if ( s_icon == 0 )
        s_icon = new QPixmap( ":/hatchet-account/hatchet-icon-512x512.png" );
#endif
}


HatchetAccountFactory::~HatchetAccountFactory()
{
}


QPixmap
HatchetAccountFactory::icon() const
{
    return *s_icon;
}


Account*
HatchetAccountFactory::createAccount( const QString& pluginId )
{
    return new HatchetAccount( pluginId.isEmpty() ? generateId( factoryId() ) : pluginId );
}


// Hatchet account

HatchetAccount::HatchetAccount( const QString& accountId )
    : Account( accountId )
    , m_publicKey( nullptr )
{
    s_instance = this;

    setAccountServiceName( "Hatchet" );
    // We're connecting peers.
    setTypes( SipType );
/*
    QFile pemFile( ":/hatchet-account/mandella.pem" );
    pemFile.open( QIODevice::ReadOnly );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "certs/mandella.pem: " << pemFile.readAll();
    pemFile.close();
    pemFile.open( QIODevice::ReadOnly );
    QCA::ConvertResult conversionResult;
    QCA::PublicKey publicKey = QCA::PublicKey::fromPEM(pemFile.readAll(), &conversionResult);
    if ( QCA::ConvertGood != conversionResult )
    {
        tLog() << Q_FUNC_INFO << "INVALID PUBKEY READ";
        return;
    }
    m_publicKey = new QCA::PublicKey( publicKey );
*/
}


HatchetAccount::~HatchetAccount()
{

}


HatchetAccount*
HatchetAccount::instance()
{
    return s_instance;
}


AccountConfigWidget*
HatchetAccount::configurationWidget()
{
    if ( m_configWidget.isNull() )
        m_configWidget = QPointer<HatchetAccountConfig>( new HatchetAccountConfig( this ) );

    return m_configWidget.data();
}


void
HatchetAccount::authenticate()
{
    if ( connectionState() == Connected )
        return;

    if ( !refreshToken().isEmpty() )
    {
        qDebug() << "Have saved credentials with refresh token:" << refreshToken();
        if ( sipPlugin() )
            sipPlugin()->connectPlugin();
        setAccountFriendlyName( username() );
    }
    else if ( !username().isEmpty() )
    {
        setAccountFriendlyName( username() );
        // Need to re-prompt for password, since we don't save it!
    }
}


void
HatchetAccount::deauthenticate()
{
    if ( !m_tomahawkSipPlugin.isNull() )
        m_tomahawkSipPlugin->disconnectPlugin();
    emit deauthenticated();
}


void
HatchetAccount::setConnectionState( Account::ConnectionState connectionState )
{
    m_state = connectionState;

    emit connectionStateChanged( connectionState );
}


Account::ConnectionState
HatchetAccount::connectionState() const
{
    return m_state;
}


SipPlugin*
HatchetAccount::sipPlugin( bool create )
{
    if ( m_tomahawkSipPlugin.isNull() )
    {
        if ( !create )
            return 0;

        tLog() << Q_FUNC_INFO;
        m_tomahawkSipPlugin = QPointer< HatchetSipPlugin >( new HatchetSipPlugin( this ) );

        return m_tomahawkSipPlugin.data();
    }
    return m_tomahawkSipPlugin.data();
}


QPixmap
HatchetAccount::icon() const
{
    return *s_icon;
}


bool
HatchetAccount::isAuthenticated() const
{
    return credentials().contains( "refresh_token" );
}


QString
HatchetAccount::username() const
{
    return credentials().value( "username" ).toString();
}


QByteArray
HatchetAccount::refreshToken() const
{
    return credentials().value( "refresh_token" ).toByteArray();
}


uint
HatchetAccount::refreshTokenExpiration() const
{
    bool ok;
    return credentials().value( "refresh_token_expiration" ).toUInt( &ok );
}


QByteArray
HatchetAccount::bearerToken() const
{
    return credentials().value( "mandella_access_token" ).toByteArray();
}


uint
HatchetAccount::bearerTokenExpiration() const
{
    bool ok;
    return credentials().value( "mandella_access_token_expiration" ).toUInt( &ok );
}


QByteArray
HatchetAccount::bearerTokenType() const
{
    return credentials().value( "mandella_token_type" ).toByteArray();
}


void
HatchetAccount::loginWithPassword( const QString& username, const QString& password, const QString &otp )
{
    //if ( username.isEmpty() || password.isEmpty() || !m_publicKey )
    if ( username.isEmpty() || password.isEmpty() )
    {
        tLog() << "No tomahawk account username or pw or public key, not logging in";
        return;
    }

    /*
    m_uuid = QUuid::createUuid().toString();
    QCA::SecureArray sa( m_uuid.toLatin1() );
    QCA::SecureArray result = m_publicKey->encrypt( sa, QCA::EME_PKCS1_OAEP );
    params[ "nonce" ] = QString( result.toByteArray().toBase64() );
    */

    QNetworkRequest req( QUrl( c_loginServer + "/authentication/password") );
    req.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );

    QUrl params;
    TomahawkUtils::urlAddQueryItem( params, "username", username );
    TomahawkUtils::urlAddQueryItem( params, "password", password );
    TomahawkUtils::urlAddQueryItem( params, "grant_type", "password" );
    if ( !otp.isEmpty() )
        TomahawkUtils::urlAddQueryItem( params, "otp", otp );

    QByteArray data = TomahawkUtils::encodedQuery( params );

    QNetworkReply* reply = Tomahawk::Utils::nam()->post( req, data );

    NewClosure( reply, SIGNAL( finished() ), this, SLOT( onPasswordLoginFinished( QNetworkReply*, const QString& ) ), reply, username );
}


void
HatchetAccount::onPasswordLoginFinished( QNetworkReply* reply, const QString& username )
{
    tLog() << Q_FUNC_INFO;
    Q_ASSERT( reply );

    reply->deleteLater();

    bool ok;
    int statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt( &ok );
    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error finding status code from auth server";
        emit authError( "An error occurred getting the status code from the server", 0, QVariantMap() );
        return;
    }
    const QVariantMap resp = HatchetHelpers::parseReply( reply, ok );
    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error getting parsed reply from auth server";
        emit authError( "An error occurred reading the reply from the authentication server", statusCode, resp );
        return;
    }
    if ( statusCode >= 500 )
    {
        tLog() << Q_FUNC_INFO << "Encountered internal error from auth server, cannot continue";
        emit authError( "The authentication server reported an internal error, please try again later", statusCode, resp );
        return;
    }
    if ( statusCode >= 400 )
    {
        QString errString = resp.value( "error_description" ).toString();
        tLog() << Q_FUNC_INFO << "An error was returned from the authentication server: " << errString;
        emit authError( errString, statusCode, resp );
        return;
    }

    /*
    const QString nonce = resp.value( "result" ).toMap().value( "nonce" ).toString();
    if ( nonce != m_uuid )
    {
        tLog() << Q_FUNC_INFO << "Auth server nonce value does not match!";
        emit authError( "The nonce value was incorrect. YOUR ACCOUNT MAY BE COMPROMISED.", statusCode, resp );
        return;
    }
    */

    const QByteArray refreshTokenBytes = resp.value( "refresh_token" ).toByteArray();
    uint refreshTokenExpiration = resp.value( "refresh_token_expires_in" ).toUInt( &ok );
    if ( refreshTokenBytes.isEmpty() || !ok )
    {
        tLog() << Q_FUNC_INFO << "Error reading refresh token or its expiration";
        emit authError( "An error encountered parsing the authentication server's response", 0, QVariantMap() );
        return;
    }
    const QByteArray accessTokenBytes = resp.value( "access_token" ).toByteArray();
    uint accessTokenExpiration = resp.value( "expires_in" ).toUInt( &ok );
    if ( accessTokenBytes.isEmpty() || !ok )
    {
        tLog() << Q_FUNC_INFO << "Error reading access token or its expiration";
        emit authError( "An error encountered parsing the authentication server's response", 0, QVariantMap() );
        return;
    }
    const QByteArray tokenTypeBytes = resp.value( "token_type" ).toByteArray();
    if ( tokenTypeBytes.isEmpty() )
    {
        tLog() << Q_FUNC_INFO << "Error reading access token type";
        emit authError( "An error encountered parsing the authentication server's response", 0, QVariantMap() );
        return;
    }

    QVariantHash creds = credentials();
    creds[ "username" ] = username;
    creds[ "refresh_token" ] = refreshTokenBytes;
    creds[ "refresh_token_expiration" ] = refreshTokenExpiration == 0 ? 0 : QDateTime::currentDateTime().toTime_t() + refreshTokenExpiration;
    creds[ "mandella_access_token" ] = accessTokenBytes;
    creds[ "mandella_access_token_expiration" ] = QDateTime::currentDateTime().toTime_t() + accessTokenExpiration;
    creds[ "mandella_token_type" ] = tokenTypeBytes;
    setCredentials( creds );
    syncConfig();

    if ( sipPlugin() )
        sipPlugin()->connectPlugin();
}


void
HatchetAccount::fetchAccessToken( const QString& type )
{
    QVariantHash creds = credentials();
    HatchetHelpers::MandellaInformation mandellaInfo;
    mandellaInfo.bearerToken = bearerToken();
    mandellaInfo.bearerTokenExpiration = bearerTokenExpiration();
    mandellaInfo.bearerTokenType = bearerTokenType();
    mandellaInfo.refreshToken = refreshToken();
    mandellaInfo.refreshTokenExpiration = refreshTokenExpiration();

    HatchetHelpers::HatchetCredentialFetcher* fetcher = new HatchetHelpers::HatchetCredentialFetcher( this );
    connect( fetcher,
        SIGNAL( mandellaInformationUpdated( const HatchetHelpers::MandellaInformation& ) ),
        SLOT( onFetcherMandellaInformationUpdated( const HatchetHelpers::MandellaInformation& ) ) );
    connect( fetcher,
        SIGNAL( authError( const QString&, int, const QVariantMap& ) ),
        SLOT( onFetcherAuthError( const QString&, int, const QVariantMap& ) ) );
    connect( fetcher,
        SIGNAL( accessTokenFetched( const HatchetHelpers::AccessTokenInformation& ) ),
        SLOT( onFetcherAccessTokenFetched( const HatchetHelpers::AccessTokenInformation& ) ) );

    fetcher->fetchAccessToken( type, mandellaInfo );
}


void
HatchetAccount::onFetcherMandellaInformationUpdated( const HatchetHelpers::MandellaInformation& mandellaInformation )
{
    QVariantHash creds = credentials();
    creds[ "mandella_access_token" ] = mandellaInformation.bearerToken;
    creds[ "mandella_access_token_expiration" ] = QDateTime::currentDateTime().toTime_t() + mandellaInformation.bearerTokenExpiration;
    creds[ "mandella_token_type" ] = mandellaInformation.bearerTokenType;
    setCredentials( creds );
    syncConfig();
}


void
HatchetAccount::onFetcherAuthError( const QString& error, int code, const QVariantMap& resp )
{
    if ( sender() )
        sender()->deleteLater();
    emit authError( error, code, resp );
}


void
HatchetAccount::onFetcherAccessTokenFetched( const HatchetHelpers::AccessTokenInformation& accessTokenInfo )
{
    if ( sender() )
        sender()->deleteLater();
    emit accessTokenFetched( accessTokenInfo );
}


}
}
}


Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::Hatchet::HatchetAccountFactory )
