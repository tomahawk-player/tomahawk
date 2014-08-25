/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef TOMAHAWK_TESTDATABASE_H
#define TOMAHAWK_TESTDATABASE_H

#include <QtTest>

#include "network/Servent.h"
#include "sip/SipInfo.h"

class TestServent : public QObject
{
    Q_OBJECT
private:

    bool plainIPAddress( const QString& address )
    {
        // TODO: Add real checks
        return true;
    }

private slots:
    void testListenAll()
    {
        // Instatiante a new instance for each test so we have a sane state.
        Servent* servent = new Servent();
        QVERIFY( servent != NULL );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
        QHostAddress anyAddress = QHostAddress::Any;
#else
        QHostAddress anyAddress = QHostAddress::AnyIPv6;
#endif

        // TODO: Use a random free port for tests
        // With (upnp == false) and (mode ==
        // Tomahawk::Network::ExternalAddress::Upnp) we should not do
        // any external address detection.
        bool ok = servent->startListening( anyAddress, false, 52222,
             Tomahawk::Network::ExternalAddress::Upnp, 52222);
        QVERIFY( ok );

        // Verify that computed external addresses are ok
        QList<QHostAddress> externalAddresses = servent->addresses();
        foreach ( QHostAddress addr, externalAddresses )
        {
            QVERIFY( plainIPAddress( addr.toString() ) );
        }

        // Verify that the local SipInfos contain valid addresses
        QList<SipInfo> sipInfos = servent->getLocalSipInfos( uuid(), uuid() );
        foreach ( SipInfo sipInfo, sipInfos )
        {
            QVERIFY( plainIPAddress( sipInfo.host() ) );
        }

        delete servent;
    }
};

#endif // TOMAHAWK_TESTDATABASE_H
