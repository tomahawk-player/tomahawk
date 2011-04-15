/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "jabber.h"

#include "tomahawksettings.h"

#include <QtPlugin>
#include <QStringList>
#include <QInputDialog>
#include <QLineEdit>


JabberPlugin::JabberPlugin()
    : p( 0 ),
    m_menu ( 0 ),
    m_addFriendAction( 0 )
{
}


void
JabberPlugin::setProxy( QNetworkProxy* proxy )
{
    p->setProxy( proxy );
}


const QString
JabberPlugin::name()
{
    return QString( MYNAME );
}


const QString
JabberPlugin::friendlyName()
{
    return QString( "Jabber" );
}


const QString
JabberPlugin::accountName()
{
    return TomahawkSettings::instance()->jabberUsername();
}


QMenu*
JabberPlugin::menu()
{
    return m_menu;
}


bool
JabberPlugin::connectPlugin( bool startup )
{
    qDebug() << "JabberPlugin::connect";
    if ( startup && !TomahawkSettings::instance()->jabberAutoConnect() )
        return false;

    m_currentUsername = TomahawkSettings::instance()->jabberUsername();
    m_currentServer = TomahawkSettings::instance()->jabberServer();
    m_currentPassword = TomahawkSettings::instance()->jabberPassword();
    m_currentPort = TomahawkSettings::instance()->jabberPort();
    QString server = m_currentServer;

    QStringList splitJid = m_currentUsername.split( '@', QString::SkipEmptyParts );
    if ( splitJid.size() < 2 )
    {
        qDebug() << "JID did not have an @ in it, could not find a server part";
        return false;
    }

    if ( server.isEmpty() )
        server = splitJid[1];

    if ( m_currentPort < 1 || m_currentPort > 65535 || m_currentUsername.isEmpty() || m_currentPassword.isEmpty() )
    {
        qDebug() << "Jabber credentials look wrong, not connecting";
        return false;
    }

    delete p;
    p = new Jabber_p( m_currentUsername, m_currentPassword, server, m_currentPort );

    QObject::connect( p, SIGNAL( peerOnline( QString ) ), SIGNAL( peerOnline( QString ) ) );
    QObject::connect( p, SIGNAL( peerOffline( QString ) ), SIGNAL( peerOffline( QString ) ) );
    QObject::connect( p, SIGNAL( msgReceived( QString, QString ) ), SIGNAL( msgReceived( QString, QString ) ) );

    QObject::connect( p, SIGNAL( connected() ), SLOT( onConnected() ) );
    QObject::connect( p, SIGNAL( disconnected() ), SLOT( onDisconnected() ) );
    QObject::connect( p, SIGNAL( authError( int, QString ) ), SLOT( onAuthError( int, QString ) ) );

    p->resolveHostSRV();

    return true;
}


void
JabberPlugin::onConnected()
{
    if ( !m_menu )
    {
        m_menu = new QMenu( tr( "Jabber (%1)" ).arg( accountName() ) );
        m_addFriendAction = m_menu->addAction( tr( "Add Friend..." ) );

        connect( m_addFriendAction, SIGNAL( triggered() ), SLOT( showAddFriendDialog() ) ) ;

        emit addMenu( m_menu );
    }

    emit connected();
}


void
JabberPlugin::onDisconnected()
{
    if ( m_menu && m_addFriendAction )
    {
        emit removeMenu( m_menu );

        delete m_menu;
        m_menu = 0;
        m_addFriendAction = 0;
    }

    emit disconnected();
}


void
JabberPlugin::onAuthError( int code, const QString& message )
{
    if ( code == gloox::ConnAuthenticationFailed )
    {
        emit error( SipPlugin::AuthError, message );
    }
    else
    {
        emit error( SipPlugin::ConnectionError, message );
    }
}


void
JabberPlugin::showAddFriendDialog()
{
    bool ok;
    QString id = QInputDialog::getText( 0, tr( "Add Friend" ),
                                           tr( "Enter Jabber ID:" ), QLineEdit::Normal, "", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to add jabber contact to roster:" << id;
    addContact( id );
}


void
JabberPlugin::checkSettings()
{
    bool reconnect = false;
    if ( m_currentUsername != TomahawkSettings::instance()->jabberUsername() )
        reconnect = true;
    if ( m_currentPassword != TomahawkSettings::instance()->jabberPassword() )
        reconnect = true;
    if ( m_currentServer != TomahawkSettings::instance()->jabberServer() )
        reconnect = true;
    if ( m_currentPort != TomahawkSettings::instance()->jabberPort() )
        reconnect = true;

    m_currentUsername = TomahawkSettings::instance()->jabberUsername();
    m_currentPassword = TomahawkSettings::instance()->jabberPassword();
    m_currentServer = TomahawkSettings::instance()->jabberServer();
    m_currentPort = TomahawkSettings::instance()->jabberPort();

    if ( reconnect && ( p || TomahawkSettings::instance()->jabberAutoConnect() ) )
    {
        disconnectPlugin();
        connectPlugin( false );
    }
}

Q_EXPORT_PLUGIN2( sip, JabberPlugin )
