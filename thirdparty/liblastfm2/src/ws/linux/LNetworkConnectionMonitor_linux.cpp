/*
   Copyright 2010 Last.fm Ltd.
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "LNetworkConnectionMonitor.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

LNetworkConnectionMonitor::LNetworkConnectionMonitor( QObject* parent ) :
    NetworkConnectionMonitor( parent )
{
    m_nmInterface = new QDBusInterface( QString( "org.freedesktop.NetworkManager" ),
                                        QString( "/org/freedesktop/NetworkManager" ),
                                        QString( "org.freedesktop.NetworkManager" ),
                                        QDBusConnection::systemBus(),
                                        this );

    //get current connection state
    QDBusInterface* dbusInterface = new QDBusInterface( QString( "org.freedesktop.NetworkManager" ),
                                                        QString( "/org/freedesktop/NetworkManager" ),
                                                        QString( "org.freedesktop.DBus.Properties" ),
                                                        QDBusConnection::systemBus(),
                                                        this );

    QDBusReply<QVariant> reply = dbusInterface->call( "Get", "org.freedesktop.NetworkManager", "state" );
    if ( reply.isValid() )
    {
        if ( reply.value() == Connected )
        {
            setConnected( true );
        }
        else if ( reply.value() == Disconnected )
        {
            setConnected( false );
        }
    }
    else
    {
        qDebug() << "Error: " << reply.error();
    }
    delete dbusInterface;

    //connect network manager signals
   connect( m_nmInterface, SIGNAL( StateChange( uint ) ), this, SLOT( onStateChange( uint ) ) );

}

LNetworkConnectionMonitor::~LNetworkConnectionMonitor()
{
    delete m_nmInterface;
}


void
LNetworkConnectionMonitor::onStateChange( uint newState )
{
    qDebug() << "Networkmanager state change!";
    
    if ( newState == Disconnected )
    {
       setConnected( false );
    }
    else if ( newState == Connected )
    {
       setConnected( true );
    }
}
