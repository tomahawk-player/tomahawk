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

#include <QNetworkInterface>
#include <QtTest>

#include "network/Servent.h"
#include "sip/SipInfo.h"

class TestServent : public QObject
{
    Q_OBJECT
private:

    void saneHostAddress( const QString& address )
    {
        // We do not use QHostAddress here as we use it inside our code.
        // (Do not use the same code to test and generate)

        // No loopback IPv4 addresses
        QVERIFY2( !address.startsWith( QLatin1String( "127.0.0." ) ),
            "Loopback IPv4 address detected" );
        // No IPv6 localhost address
        QVERIFY2( address != "::1", "IPv6 localhost address detected" );
        // No IPv4 localhost as IPv6 address
        QVERIFY2( address != "::7F00:1",
            "IPv4 localhost as IPv6 address detected" );
        // No link-local IPv6 addresses
        QVERIFY2( !address.startsWith( QLatin1String( "fe80::" ) ),
            "Link-local IPv6 address detected" );
    }

    void listenAllBasic( Servent** servent )
    {
        // Instantiate a new instance for each test so we have a sane state.
        *servent = new Servent();
        QVERIFY( *servent != NULL );

        QHostAddress anyAddress = QHostAddress::Any;

        // TODO: Use a random free port for tests
        // With (upnp == false) and (mode ==
        // Tomahawk::Network::ExternalAddress::Upnp) we should not do
        // any external address detection.
        bool ok = (*servent)->startListening( anyAddress, false, 52222,
             Tomahawk::Network::ExternalAddress::Upnp, 52222);
        QVERIFY( ok );
    }

private slots:
    void testListenAll()
    {
        Servent* servent;
        listenAllBasic( &servent );

        // Verify that computed external addresses are ok
        QList<QHostAddress> externalAddresses = servent->addresses();
        foreach ( QHostAddress addr, externalAddresses )
        {
            saneHostAddress( addr.toString() );
        }

        // Verify that the local SipInfos contain valid addresses
        QList<SipInfo> sipInfos = servent->getLocalSipInfos( uuid(), uuid() );
        foreach ( SipInfo sipInfo, sipInfos )
        {
            saneHostAddress( sipInfo.host() );
        }

        delete servent;
    }

    void testWhitelist()
    {
        Servent* servent;
        listenAllBasic( &servent );

        // Check for IPv4 localhost
        QVERIFY( servent->isIPWhitelisted( QHostAddress::LocalHost ) );

        // Check for IPv6 localhost
        QVERIFY( servent->isIPWhitelisted( QHostAddress::LocalHostIPv6 ) );

        // Verify that all interface addresses are whitelisted.
        foreach ( QHostAddress addr, QNetworkInterface::allAddresses() )
        {
            QVERIFY( servent->isIPWhitelisted( addr ) );

            if ( addr.protocol() == QAbstractSocket::IPv4Protocol )
            {
                // Convert to IPv6 mapped address
                quint32 ipv4 = addr.toIPv4Address();
                Q_IPV6ADDR ipv6;
                for (int i = 0; i < 16; ++i) {
                    ipv6[i] = 0;
                }
                ipv6[10] = 0xff;
                ipv6[11] = 0xff;
                ipv6[12] = 0xff & (ipv4 >> 24);
                ipv6[13] = 0xff & (ipv4 >> 16);
                ipv6[14] = 0xff & (ipv4 >> 8);
                ipv6[15] = 0xff & ipv4;
                QHostAddress ipv6Addr( ipv6 );
                QString error = QString( "%1 converted to IPv6 %2" )
                    .arg( addr.toString() ).arg( ipv6Addr.toString() );
                QVERIFY2( servent->isIPWhitelisted( ipv6Addr ),
                    error.toLatin1().constData() );
            }
        }

        delete servent;
    }
};

#endif // TOMAHAWK_TESTDATABASE_H
