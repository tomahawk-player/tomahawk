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

#include "Api2HttpsServerConnector.h"

#include "utils/Logger.h"

#include <QSslSocket>


Api2HttpsServerConnector::Api2HttpsServerConnector( QObject *parent )
    : QxtHttpsServerConnector( parent )
{
}


void
Api2HttpsServerConnector::sslErrors( const QList<QSslError>& errors )
{
    QList<QSslError> toIgnore;
    QSslSocket* socket = static_cast<QSslSocket*>( sender() );
    foreach ( QSslError error, errors )
    {
        if ( error.error() == QSslError::SelfSignedCertificate )
        {
            toIgnore.append( error );
        }
        else if ( error.error() == QSslError::CertificateUntrusted )
        {
            toIgnore.append( error );
        }
        else
        {
            tLog() << Q_FUNC_INFO << "Not ignoring SSL error" << error << error.error();
        }
    }
    socket->ignoreSslErrors( toIgnore );
}
