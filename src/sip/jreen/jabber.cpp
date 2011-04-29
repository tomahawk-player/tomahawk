/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Dominik Schmidt <dev@dominik-schmidt.de>
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
#include <QMessageBox>

#include "ui_configwidget.h"

SipPlugin*
JabberFactory::createPlugin( const QString& pluginId )
{
    return new JabberPlugin( pluginId.isEmpty() ? generateId() : pluginId );
}

QIcon
JabberFactory::icon() const
{
    return QIcon( ":/jabber-icon.png" );
}


JabberPlugin::JabberPlugin( const QString& pluginId )
    : SipPlugin( pluginId )
    , p( 0 )
    , m_menu( 0 )
    , m_addFriendAction( 0 )
    , m_state( Disconnected )
{
    m_configWidget = QWeakPointer< QWidget >( new QWidget );
    m_ui = new Ui_JabberConfig;
    m_ui->setupUi( m_configWidget.data() );
    m_configWidget.data()->setVisible( false );

    m_ui->checkBoxAutoConnect->setChecked( readAutoConnect() );
    m_ui->jabberUsername->setText( accountName() );
    m_ui->jabberPassword->setText( readPassword() );
    m_ui->jabberServer->setText( readServer() );
    m_ui->jabberPort->setValue( readPort() );

}

JabberPlugin::~JabberPlugin()
{
    delete p;
}

void
JabberPlugin::setProxy( QNetworkProxy* proxy )
{
    p->setProxy( proxy );
}


const QString
JabberPlugin::name() const
{
    return QString( MYNAME );
}

const QString
JabberPlugin::friendlyName() const
{
    return QString( "Jabber" );
}

const QString
JabberPlugin::accountName() const
{
    return TomahawkSettings::instance()->value( pluginId() + "/username" ).toString();
}

QMenu*
JabberPlugin::menu()
{
    return m_menu;
}

QWidget*
JabberPlugin::configWidget()
{
    return m_configWidget.data();
}

QIcon
JabberPlugin::icon() const
{
    return QIcon( ":/jabber-icon.png" );
}


bool
JabberPlugin::connectPlugin( bool startup )
{
    qDebug() << Q_FUNC_INFO;

    if ( startup && !readAutoConnect() )
        return false;

    QString jid       = m_currentUsername = accountName();
    QString server    = m_currentServer = readServer();
    QString password  = m_currentPassword = readPassword();
    unsigned int port = m_currentPort = readPort();

    QStringList splitJid = jid.split( '@', QString::SkipEmptyParts );
    if ( splitJid.size() < 2 )
    {
        qDebug() << "JID did not have an @ in it, could not find a server part";
        return false;
    }

    if ( port < 1 || port > 65535 || jid.isEmpty() || password.isEmpty() )
    {
        qDebug() << "Jabber credentials look wrong, not connecting";
        return false;
    }

    delete p;
    p = new Jabber_p( jid, password, ( server.isEmpty() ? QString() : server ), port );

    QObject::connect( p, SIGNAL( peerOnline( QString ) ), SIGNAL( peerOnline( QString ) ) );
    QObject::connect( p, SIGNAL( peerOffline( QString ) ), SIGNAL( peerOffline( QString ) ) );
    QObject::connect( p, SIGNAL( msgReceived( QString, QString ) ), SIGNAL( msgReceived( QString, QString ) ) );

    QObject::connect( p, SIGNAL( connected() ), SLOT( onConnected() ) );
    QObject::connect( p, SIGNAL( disconnected() ), SLOT( onDisconnected() ) );

    QObject::connect( p, SIGNAL( authError( int, QString ) ), SLOT( onAuthError( int, QString ) ) );
    QObject::connect( p, SIGNAL( avatarReceived( QString, QPixmap ) ), SIGNAL( avatarReceived( QString, QPixmap ) ) );
    QObject::connect( p, SIGNAL( avatarReceived( QPixmap ) ), SIGNAL( avatarReceived( QPixmap) ) );

    m_state = Connecting;
    emit stateChanged( m_state );
    return true;
}

void
JabberPlugin::disconnectPlugin()
{
    onDisconnected();

    if ( p )
        p->disconnect();

    delete p;
    p = 0;
}

void
JabberPlugin::onConnected()
{
    if( !m_menu ) {
        m_menu = new QMenu( QString( "JREEN (" ).append( accountName() ).append(")" ) );
        m_addFriendAction = m_menu->addAction( "Add Friend..." );
        QAction *connectAction = m_menu->addAction( "Connect" );

        connect( m_addFriendAction, SIGNAL(triggered() ),
                this,              SLOT( showAddFriendDialog() ) );
        connect( connectAction, SIGNAL( triggered() ), SLOT( connectPlugin() ) );

        emit addMenu( m_menu );
    }

    m_state = Connected;

    emit stateChanged( m_state );
}

void
JabberPlugin::onDisconnected()
{
    if( m_menu && m_addFriendAction ) {
        emit removeMenu( m_menu );

        delete m_menu;
        m_menu = 0;
        m_addFriendAction = 0; // deleted by menu
    }

    m_state = Disconnected;

    emit stateChanged( m_state );
}

void
JabberPlugin::onAuthError( int code, const QString& msg )
{
    switch( code )
    {
        case Jreen::Client::AuthorizationError:
            emit error( SipPlugin::AuthError, msg );
            break;

        case Jreen::Client::HostUnknown:
        case Jreen::Client::ItemNotFound:
        case Jreen::Client::RemoteStreamError:
        case Jreen::Client::RemoteConnectionFailed:
        case Jreen::Client::InternalServerError:
        case Jreen::Client::SystemShutdown:
        case Jreen::Client::Conflict:
        case Jreen::Client::Unknown:
            emit error( SipPlugin::ConnectionError, msg );
            break;

        default:
            qDebug() << "Not all Client::DisconnectReasons checked";
            Q_ASSERT(false);
            break;
    }
    m_state = Disconnected;

    emit stateChanged( m_state );
}

void
JabberPlugin::sendMsg(const QString& to, const QString& msg)
{
    if ( p )
        p->sendMsg( to, msg );
}

void
JabberPlugin::broadcastMsg(const QString& msg)
{
    if ( p )
        p->broadcastMsg( msg );
}

void
JabberPlugin::addContact(const QString& jid, const QString& msg)
{
    if ( p )
        p->addContact( jid, msg );
}

void
JabberPlugin::showAddFriendDialog()
{
    bool ok;
    QString id = QInputDialog::getText( 0, tr( "Add Friend" ),
                                              tr( "Enter Jabber ID:" ), QLineEdit::Normal,
                                              "", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to add jabber contact to roster:" << id;
    addContact( id );
}

void
JabberPlugin::checkSettings()
{
    bool reconnect = false;
    if ( m_currentUsername != accountName() )
        reconnect = true;
    if ( m_currentPassword != readPassword() )
        reconnect = true;
    if ( m_currentServer != readServer() )
        reconnect = true;
    if ( m_currentPort != readPort() )
        reconnect = true;

    m_currentUsername = accountName();
    m_currentPassword = readPassword();
    m_currentServer = readServer();
    m_currentPort = readPort();

    if ( reconnect && ( p || readAutoConnect() ) )
    {
        disconnectPlugin();
        connectPlugin( false );
    }
}

QString
JabberPlugin::readPassword()
{
    return TomahawkSettings::instance()->value( pluginId() + "/password" ).toString();
}

int
JabberPlugin::readPort()
{
    return TomahawkSettings::instance()->value( pluginId() + "/port", 5222 ).toInt();
}

QString
JabberPlugin::readServer()
{
    return TomahawkSettings::instance()->value( pluginId() + "/server" ).toString();
}

bool
JabberPlugin::readAutoConnect()
{
    return TomahawkSettings::instance()->value( pluginId() + "/autoconnect", true ).toBool();
}

void
JabberPlugin::saveConfig()
{
    TomahawkSettings::instance()->setValue( pluginId() + "/autoconnect", m_ui->checkBoxAutoConnect->isChecked() );
    TomahawkSettings::instance()->setValue( pluginId() + "/username", m_ui->jabberUsername->text() );
    TomahawkSettings::instance()->setValue( pluginId() + "/pasword", m_ui->jabberPassword->text() );
    TomahawkSettings::instance()->setValue( pluginId() + "/port", m_ui->jabberPort->value() );
    TomahawkSettings::instance()->setValue( pluginId() + "/server", m_ui->jabberServer->text() );

    checkSettings();
}


SipPlugin::ConnectionState
JabberPlugin::connectionState() const
{
    return m_state;
}

#ifndef GOOGLE_WRAPPER
Q_EXPORT_PLUGIN2( sipfactory, JabberFactory )
#endif