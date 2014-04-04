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

#ifndef HATCHET_HELPERS_H
#define HATCHET_HELPERS_H

#include <QVariantMap>

class QNetworkReply;

namespace Tomahawk
{
namespace Accounts
{
namespace Hatchet
{
namespace HatchetHelpers
{


QVariantMap parseReply( QNetworkReply* reply, bool& ok );


struct MandellaInformation {
    QByteArray bearerTokenType;
    QByteArray bearerToken;
    uint bearerTokenExpiration;
    QByteArray refreshToken;
    uint refreshTokenExpiration;
};

struct AccessTokenInformation {
    QByteArray type;
    QByteArray token;
    uint expiration;
};


class HatchetCredentialFetcher : public QObject
{
    Q_OBJECT

public:
    HatchetCredentialFetcher( QObject* parent );
    virtual ~HatchetCredentialFetcher();

signals:
    void accessTokenFetched( const HatchetHelpers::AccessTokenInformation& accessInfo );
    void mandellaInformationUpdated( const HatchetHelpers::MandellaInformation& updatedInfo );
    void authError( const QString& error, int statusCode, const QVariantMap& resp );

public slots:
    void fetchAccessToken( const QString& type, const HatchetHelpers::MandellaInformation& mandellaInformation );

private slots:
    void onFetchAccessTokenFinished( QNetworkReply*, const QString& type );
};


}
}
}
}

#endif
