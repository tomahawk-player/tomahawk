/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "HatchetHelpers.h"

#include "utils/Closure.h"
#include "utils/Logger.h"
#include "utils/NetworkAccessManager.h"
#include "utils/TomahawkUtils.h"

#include <qjson/parser.h>

#include <QDateTime>
#include <QNetworkReply>


namespace Tomahawk
{
namespace Accounts
{
namespace Hatchet
{
namespace HatchetHelpers
{


QVariantMap
parseReply( QNetworkReply* reply, bool& okRet )
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


HatchetCredentialFetcher::HatchetCredentialFetcher( QObject* parent )
    : QObject( parent )
{
}


HatchetCredentialFetcher::~HatchetCredentialFetcher()
{

}


void
HatchetCredentialFetcher::fetchAccessToken( const QString& type, const MandellaInformation& mandellaInformation )
{
    if ( mandellaInformation.bearerToken.isEmpty() ||
        (mandellaInformation.bearerTokenExpiration < QDateTime::currentDateTime().toTime_t() &&
          (mandellaInformation.refreshToken.isEmpty() ||
            (mandellaInformation.refreshTokenExpiration != 0 && mandellaInformation.refreshTokenExpiration < QDateTime::currentDateTime().toTime_t()))) )
    {
        tLog() << "No valid combination of access/refresh tokens, not logging in";
        tLog() << "Mandella access token expiration:" << mandellaInformation.bearerTokenExpiration << ", refresh token expiration:" << mandellaInformation.refreshTokenExpiration;
        emit authError( "No valid credentials are stored locally, please log in again.", 401, QVariantMap() );
        return;
    }

    bool interceptionNeeded = mandellaInformation.bearerTokenExpiration < QDateTime::currentDateTime().toTime_t();

    QNetworkRequest req( QUrl( "https://auth.hatchet.is/v1/tokens/" + (interceptionNeeded ? "refresh/" + QString::fromUtf8(mandellaInformation.bearerTokenType).toLower() : "fetch/" + type) ) );
    QNetworkReply* reply;

    if ( interceptionNeeded )
    {
        tLog() << "Intercepting; new mandella access token needed";
        req.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
        QUrl params;
        TomahawkUtils::urlAddQueryItem( params, "grant_type", "refresh_token" );
        TomahawkUtils::urlAddQueryItem( params, "refresh_token", mandellaInformation.refreshToken );
        QByteArray data = TomahawkUtils::encodedQuery( params );
        reply = Tomahawk::Utils::nam()->post( req, data );
        reply->setProperty( "originalType", type );
    }
    else
    {
        tLog() << "Fetching token of type" << type;
        req.setRawHeader( "Authorization", QString( mandellaInformation.bearerTokenType + " " + mandellaInformation.bearerToken).toUtf8() );
        reply = Tomahawk::Utils::nam()->get( req );
    }

    NewClosure( reply, SIGNAL( finished() ), this, SLOT( onFetchAccessTokenFinished( QNetworkReply*, const QString& ) ), reply, type );
}


void
HatchetCredentialFetcher::onFetchAccessTokenFinished( QNetworkReply* reply, const QString& type )
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

    if ( !originalType.isEmpty() )
    {
        const QByteArray bearerTokenBytes = resp.value( "access_token" ).toByteArray();
        uint bearerTokenExpiration = resp.value( "expires_in" ).toUInt( &ok );
        if ( bearerTokenBytes.isEmpty() || !ok )
        {
            tLog() << Q_FUNC_INFO << "Error reading access token or its expiration";
            emit authError( "An error encountered parsing the authentication server's response", 0, QVariantMap() );
            return;
        }
        const QByteArray bearerTokenTypeBytes = resp.value( "token_type" ).toByteArray();
        if ( bearerTokenTypeBytes.isEmpty() )
        {
            tLog() << Q_FUNC_INFO << "Error reading access token type";
            emit authError( "An error encountered parsing the authentication server's response", 0, QVariantMap() );
            return;
        }

        MandellaInformation newMandellaInformation;
        newMandellaInformation.bearerTokenType = bearerTokenTypeBytes;
        newMandellaInformation.bearerToken = bearerTokenBytes;
        newMandellaInformation.bearerTokenExpiration = QDateTime::currentDateTime().toTime_t() + bearerTokenExpiration;
        emit mandellaInformationUpdated( newMandellaInformation );

        fetchAccessToken( originalType, newMandellaInformation );
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

    tLog() << Q_FUNC_INFO << "Access tokens fetched successfully";

    AccessTokenInformation accessTokenInformation;
    accessTokenInformation.type = tokenTypeBytes;
    accessTokenInformation.token = accessTokenBytes;
    accessTokenInformation.expiration = accessTokenExpiration;

    emit accessTokenFetched( accessTokenInformation );
}


}
}
}
}
