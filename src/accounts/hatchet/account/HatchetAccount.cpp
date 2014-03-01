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
#include <QHostInfo>

#include "HatchetAccountConfig.h"
#include "utils/Closure.h"
#include "utils/Logger.h"
#include "sip/HatchetSip.h"
#include "utils/TomahawkUtils.h"
#include "utils/NetworkAccessManager.h"

#include <QtPlugin>
#include <QFile>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QUuid>

#include <qjson/parser.h>
#include <qjson/serializer.h>

using namespace Tomahawk;
using namespace Accounts;

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
        connect( m_tomahawkSipPlugin.data(), SIGNAL( authUrlDiscovered( Tomahawk::Accounts::HatchetAccount::Service, QString ) ),
                 this, SLOT( authUrlDiscovered( Tomahawk::Accounts::HatchetAccount::Service, QString ) ) );

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
HatchetAccount::mandellaAccessToken() const
{
    return credentials().value( "mandella_access_token" ).toByteArray();
}


uint
HatchetAccount::mandellaAccessTokenExpiration() const
{
    bool ok;
    return credentials().value( "mandella_access_token_expiration" ).toUInt( &ok );
}


QByteArray
HatchetAccount::mandellaTokenType() const
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
    params.addQueryItem( "username", username );
    params.addQueryItem( "password", password );
    params.addQueryItem( "grant_type", "password" );
    if ( !otp.isEmpty() )
        params.addQueryItem( "otp", otp );

    QByteArray data = params.encodedQuery();

    QNetworkReply* reply = Tomahawk::Utils::nam()->post( req, data );

    NewClosure( reply, SIGNAL( finished() ), this, SLOT( onPasswordLoginFinished( QNetworkReply*, const QString& ) ), reply, username );
}


void
HatchetAccount::fetchAccessToken( const QString& type )
{
    if ( username().isEmpty() )
    {
        tLog() << "No username, not logging in";
        return;
    }
    if ( mandellaAccessToken().isEmpty() ||
        (mandellaAccessTokenExpiration() < QDateTime::currentDateTime().toTime_t() &&
          (refreshToken().isEmpty() ||
            (refreshTokenExpiration() != 0 && refreshTokenExpiration() < QDateTime::currentDateTime().toTime_t()))) )
    {
        tLog() << "No valid combination of access/refresh tokens, not logging in";
        tLog() << "Mandella access token expiration:" << mandellaAccessTokenExpiration() << ", refresh token expiration:" << refreshTokenExpiration();
        emit authError( "No valid credentials are stored locally, please log in again.", 401, QVariantMap() );
        return;
    }

    uint matExpiration = mandellaAccessTokenExpiration();
    bool interceptionNeeded = false;

    if ( matExpiration < QDateTime::currentDateTime().toTime_t() )
    {
        interceptionNeeded = true;
        tLog() << "Mandella access token has expired, fetching new ones first";
    }
    else
    {
        tLog() << "Fetching access tokens of type" << type;
    }

    QNetworkRequest req( QUrl( c_accessTokenServer + "/tokens/" + (interceptionNeeded ? "refresh/" + QString::fromUtf8(mandellaTokenType()).toLower() : "fetch/" + type) ) );
    QNetworkReply* reply;

    if ( interceptionNeeded )
    {
        tLog() << "Intercepting; new mandella access token needed";
        req.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
        QUrl params;
        params.addQueryItem( "grant_type", "refresh_token" );
        params.addQueryItem( "refresh_token", refreshToken() );
        QByteArray data = params.encodedQuery();
        reply = Tomahawk::Utils::nam()->post( req, data );
        reply->setProperty( "originalType", type );
    }
    else
    {
        tLog() << "Fetching token of type" << type;
        req.setRawHeader( "Authorization", QString( mandellaTokenType() + " " + mandellaAccessToken()).toUtf8() );
        reply = Tomahawk::Utils::nam()->get( req );
    } 

    NewClosure( reply, SIGNAL( finished() ), this, SLOT( onFetchAccessTokenFinished( QNetworkReply*, const QString& ) ), reply, type );
}


void
HatchetAccount::onPasswordLoginFinished( QNetworkReply* reply, const QString& username )
{
    tLog() << Q_FUNC_INFO;
    Q_ASSERT( reply );
    bool ok;
    int statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt( &ok );
    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error finding status code from auth server";
        emit authError( "An error occurred getting the status code from the server", 0, QVariantMap() );
        return;
    }
    const QVariantMap resp = parseReply( reply, ok );
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
HatchetAccount::onFetchAccessTokenFinished( QNetworkReply* reply, const QString& type )
{
    tLog() << Q_FUNC_INFO;
    Q_ASSERT( reply );

    QString originalType;
    if ( reply->property( "originalType" ).isValid() )
    {
        originalType = reply->property( "originalType" ).toString();
    }

    bool ok;
    int statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt( &ok );
    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error finding status code from auth server";
        emit authError( "An error occurred getting the status code from the server", 0, QVariantMap() );
        return;
    }
    const QVariantMap resp = parseReply( reply, ok );
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

    QVariantHash creds = credentials();

    if ( !originalType.isEmpty() )
    {
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
        creds[ "mandella_access_token" ] = accessTokenBytes;
        creds[ "mandella_access_token_expiration" ] = QDateTime::currentDateTime().toTime_t() + accessTokenExpiration;
        creds[ "mandella_token_type" ] = tokenTypeBytes;
        setCredentials( creds );
        syncConfig();

        fetchAccessToken( originalType );
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

    creds[ type + "_access_token" ] = accessTokenBytes;

    tDebug() << Q_FUNC_INFO << "Creds: " << creds;

    setCredentials( creds );
    syncConfig();

    tLog() << Q_FUNC_INFO << "Access tokens fetched successfully";

    emit accessTokenFetched();
}


QString
HatchetAccount::authUrlForService( const Service &service ) const
{
    return m_extraAuthUrls.value( service, QString() );
}


void
HatchetAccount::authUrlDiscovered( Service service, const QString &authUrl )
{
    m_extraAuthUrls[ service ] = authUrl;
}


QVariantMap
HatchetAccount::parseReply( QNetworkReply* reply, bool& okRet ) const
{
    QVariantMap resp;

    reply->deleteLater();

    bool ok;
    int statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt( &ok );
    if ( reply->error() != QNetworkReply::NoError && statusCode < 400 )
    {
        tLog() << Q_FUNC_INFO << "Network error in command:" << reply->error() << reply->errorString();
        okRet = false;
        return resp;
    }

    QJson::Parser p;
    QByteArray replyData = reply->readAll();
    resp = p.parse( replyData, &ok ).toMap();

    if ( !ok )
    {
        tLog() << Q_FUNC_INFO << "Error parsing JSON from server" << replyData;
        okRet = false;
        return resp;
    }

    if ( statusCode >= 400 )
    {
        tDebug() << "Error from tomahawk server response, or in parsing from json:" << resp.value( "error" ).toString() << resp;
    }

    tDebug() << Q_FUNC_INFO << "Got keys" << resp.keys();
    tDebug() << Q_FUNC_INFO << "Got values" << resp.values();
    okRet = true;
    return resp;
}

Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::HatchetAccountFactory )
