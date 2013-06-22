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

#ifndef API2HTTPSSERVERCONNECTOR_H
#define API2HTTPSSERVERCONNECTOR_H

#include <QxtWeb/QxtHttpServerConnector>

class Api2HttpsServerConnector : public QxtHttpsServerConnector
{
    Q_OBJECT
public:
    Api2HttpsServerConnector( QObject* parent );

protected:
    /**
     * We want to handle ourselves which SSL certificates are trusted.
     *
     * Ignore errors about untrusted and self-signed certificates.
     *
     * Already a slot in QxtHttpsServerConnector
     */
    void sslErrors( const QList<QSslError>& errors );
    
};

#endif // API2HTTPSSERVERCONNECTOR_H
