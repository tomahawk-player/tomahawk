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

#include "config.h"

#include "tomahawksettings.h"
#include "tomahawksipmessage.h"
#include "tomahawksipmessagefactory.h"

#include <jreen/jid.h>
#include <jreen/capabilities.h>
#include <jreen/vcardupdate.h>
#include <jreen/vcard.h>
#include <jreen/directconnection.h>
#include <jreen/tcpconnection.h>

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QtPlugin>
#include <QStringList>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QDateTime>
#include <QTimer>

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
    , m_menu( 0 )
    , m_addFriendAction( 0 )
    , m_state( Disconnected )
{
    qDebug() << Q_FUNC_INFO;

    qsrand(QDateTime::currentDateTime().toTime_t());

    m_configWidget = QWeakPointer< QWidget >( new QWidget );
    m_ui = new Ui_JabberConfig;
    m_ui->setupUi( m_configWidget.data() );
    m_configWidget.data()->setVisible( false );

    m_ui->jabberUsername->setText( accountName() );
    m_ui->jabberPassword->setText( readPassword() );
    m_ui->jabberServer->setText( readServer() );
    m_ui->jabberPort->setValue( readPort() );
    m_currentUsername = accountName();
    m_currentServer = readServer();
    m_currentPassword = readPassword();
    m_currentPort = readPort();

    // setup JID object
    Jreen::JID jid = Jreen::JID( accountName() );

    // general client setup
    m_client = new Jreen::Client( jid, m_currentPassword );
    setupClientHelper();

    m_client->registerStanzaExtension(new TomahawkSipMessageFactory);
    m_currentResource = QString::fromAscii( "tomahawk%1" ).arg( QString::number( qrand() % 10000 ) );
    m_client->setResource( m_currentResource );

    // add VCardUpdate extension to own presence
    m_client->presence().addExtension( new Jreen::VCardUpdate() );

    // initaliaze the roster
    m_roster = new Jreen::SimpleRoster( m_client );

    // initialize the AvatarManager
    m_avatarManager = new AvatarManager( m_client );

    // setup disco
    m_client->disco()->setSoftwareVersion( "Tomahawk Player", TOMAHAWK_VERSION, CMAKE_SYSTEM );
    m_client->disco()->addIdentity( Jreen::Disco::Identity( "client", "type", "tomahawk", "en" ) );
    m_client->disco()->addFeature( TOMAHAWK_FEATURE );

    // setup caps node, legacy peer detection - used before 0.1
    Jreen::Capabilities::Ptr caps = m_client->presence().findExtension<Jreen::Capabilities>();
    caps->setNode( TOMAHAWK_CAP_NODE_NAME );
    //FIXME: caps->setVersion( TOMAHAWK_VERSION );

    // print connection parameters
    qDebug() << "Our JID set to:" << m_client->jid().full();
    qDebug() << "Our Server set to:" << m_client->server();
    qDebug() << "Our Port set to" << m_client->port();

    // setup slots
    connect(m_client, SIGNAL(serverFeaturesReceived(QSet<QString>)), SLOT(onConnect()));
    connect(m_client, SIGNAL(disconnected(Jreen::Client::DisconnectReason)), SLOT(onDisconnect(Jreen::Client::DisconnectReason)));
    connect(m_client, SIGNAL(newMessage(Jreen::Message)), SLOT(onNewMessage(Jreen::Message)));

    connect(m_client, SIGNAL(newIQ(Jreen::IQ)), SLOT(onNewIq(Jreen::IQ)));

    connect(m_roster, SIGNAL(presenceReceived(Jreen::RosterItem::Ptr,Jreen::Presence)),
                      SLOT(onPresenceReceived(Jreen::RosterItem::Ptr,Jreen::Presence)));
    connect(m_roster, SIGNAL(subscriptionReceived(Jreen::RosterItem::Ptr,Jreen::Presence)),
                      SLOT(onSubscriptionReceived(Jreen::RosterItem::Ptr,Jreen::Presence)));

    connect(m_avatarManager, SIGNAL(newAvatar(QString)), SLOT(onNewAvatar(QString)));
}

JabberPlugin::~JabberPlugin()
{

}

void
JabberPlugin::setProxy( const QNetworkProxy &proxy )
{
    qDebug() << Q_FUNC_INFO;

    if( ( proxy.type() != QNetworkProxy::NoProxy ) && ( m_currentServer.isEmpty() || !(m_currentPort > 0) ) )
    {
        // patches are welcome in Jreen that implement jdns through proxy
        emit error( SipPlugin::ConnectionError,
                    tr( "You need to set hostname and port of your jabber server, if you want to use it through a proxy" ) );
        return;
    }

    if(!m_client->connection())
    {
        m_client->setConnection(new Jreen::TcpConnection(m_currentServer, m_currentPort));
    }

    qobject_cast<Jreen::DirectConnection*>(m_client->connection())->setProxy(proxy);
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

    if(m_client->isConnected())
    {
        qDebug() << Q_FUNC_INFO << "Already connected to server, not connecting again...";
        return true; //FIXME: should i return false here?!
    }

    qDebug() << "Connecting to the XMPP server...";

    qDebug() << m_client->jid().full();
    qDebug() << m_client->server() << m_client->port();

    //FIXME: we're badly workarounding some missing reconnection api here, to be fixed soon
    QTimer::singleShot( 1000, m_client, SLOT( connectToServer() ) );


    connect(m_client->connection(), SIGNAL(error(Jreen::Connection::SocketError)), SLOT(onError(Jreen::Connection::SocketError)));

    m_state = Connecting;
    emit stateChanged( m_state );
    return true;
}

void
JabberPlugin::disconnectPlugin()
{
    if (!m_client->isConnected())
    {
        if ( m_state != Disconnected ) // might be Connecting
        {
           m_state = Disconnected;
           emit stateChanged( m_state );
        }
        return;
    }

    //m_roster->deleteLater();
    //m_roster = 0;
    //m_room->deleteLater();
    //m_room = 0;

    m_peers.clear();
    m_legacy_peers.clear();

    m_client->disconnectFromServer(true);
}

void
JabberPlugin::onConnect()
{
    qDebug() << Q_FUNC_INFO;

    // update jid resource, servers like gtalk use resource binding and may
    // have changed our requested /resource
    if ( m_client->jid().resource() != m_currentResource )
    {
        m_currentResource = m_client->jid().resource();
        emit jidChanged( m_client->jid().full() );
    }

    qDebug() << "Connected as:" << m_client->jid().full();

    // set presence to least valid value
    m_client->setPresence(Jreen::Presence::XA, "Got Tomahawk? http://gettomahawk.com", -127);

    // set ping timeout to 15 secs (TODO: verify if this works)
    m_client->setPingInterval(1000);

    // load roster
    m_roster->load();

    //FIXME: this implementation is totally broken atm, so it's disabled to avoid harm :P
    // join MUC with bare jid as nickname
    //TODO: make the room a list of rooms and make that configurable
    QString mucNickname = QString( "tomahawk@conference.qutim.org/" ).append( QString( m_client->jid().bare() ).replace( "@", "-" ) );
    //m_room = new Jreen::MUCRoom(m_client, Jreen::JID( mucNickname ) );
    //m_room->setHistorySeconds(0);
    //m_room->join();

    // treat muc participiants like contacts
    //connect( m_room, SIGNAL( messageReceived( Jreen::Message, bool ) ), this, SLOT( onNewMessage( Jreen::Message ) ) );
    //connect( m_room, SIGNAL( presenceReceived( Jreen::Presence, const Jreen::MUCRoom::Participant* ) ), this, SLOT( onNewPresence( Jreen::Presence ) ) );

    m_state = Connected;
    emit stateChanged( m_state );

    addMenuHelper();
}

void
JabberPlugin::onDisconnect( Jreen::Client::DisconnectReason reason )
{
    qDebug() << Q_FUNC_INFO;

    switch( reason )
    {
        case Jreen::Client::User:
            break;

        case Jreen::Client::AuthorizationError:
            emit error( SipPlugin::AuthError, errorMessage( reason ) );
            break;

        case Jreen::Client::HostUnknown:
        case Jreen::Client::ItemNotFound:
        case Jreen::Client::RemoteStreamError:
        case Jreen::Client::RemoteConnectionFailed:
        case Jreen::Client::InternalServerError:
        case Jreen::Client::SystemShutdown:
        case Jreen::Client::Conflict:
        case Jreen::Client::Unknown:
            emit error( SipPlugin::ConnectionError, errorMessage( reason ) );
            break;

        default:
            qDebug() << "Not all Client::DisconnectReasons checked";
            Q_ASSERT(false);
            break;
    }
    m_state = Disconnected;
    emit stateChanged( m_state );

    removeMenuHelper();

    Q_FOREACH(const Jreen::JID &peer, m_peers.keys())
    {
        handlePeerStatus(peer, Jreen::Presence::Unavailable);
    }
}

QString
JabberPlugin::errorMessage( Jreen::Client::DisconnectReason reason )
{
    switch( reason )
    {
        case Jreen::Client::User:
            return tr("User Interaction");
            break;
        case Jreen::Client::HostUnknown:
            return tr("Host is unknown");
            break;
        case Jreen::Client::ItemNotFound:
            return tr("Item not found");
            break;
        case Jreen::Client::AuthorizationError:
            return tr("Authorization Error");
            break;
        case Jreen::Client::RemoteStreamError:
            return tr("Remote Stream Error");
            break;
        case Jreen::Client::RemoteConnectionFailed:
            return tr("Remote Connection failed");
            break;
        case Jreen::Client::InternalServerError:
            return tr("Internal Server Error");
            break;
        case Jreen::Client::SystemShutdown:
            return tr("System shutdown");
            break;
        case Jreen::Client::Conflict:
            return tr("Conflict");
            break;

        case Jreen::Client::Unknown:
            return tr("Unknown");
            break;

        default:
            qDebug() << "Not all Client::DisconnectReasons checked";
            Q_ASSERT(false);
            break;
    }

    m_state = Disconnected;
    emit stateChanged( m_state );

    return QString();
}

void
JabberPlugin::sendMsg(const QString& to, const QString& msg)
{
    qDebug() << Q_FUNC_INFO << to << msg;

    if ( !m_client ) {
        return;
    }

    if( m_legacy_peers.contains( to ) )
    {
        qDebug() << Q_FUNC_INFO << to << "Send legacy message" << msg;
        Jreen::Message m( Jreen::Message::Chat, Jreen::JID(to), msg);
        m_client->send( m );

        return;
    }


    /*******************************************************
     * Obsolete this by a SipMessage class
     */
    QJson::Parser parser;
    bool ok;
    QVariant v = parser.parse( msg.toAscii(), &ok );
    if ( !ok  || v.type() != QVariant::Map )
    {
        qDebug() << "Invalid JSON in XMPP msg";
        return;
    }
    QVariantMap m = v.toMap();
    /*******************************************************/

    TomahawkSipMessage *sipMessage;
    if(m["visible"].toBool())
    {
        sipMessage = new TomahawkSipMessage(m["ip"].toString(),
                                            m["port"].toInt(),
                                            m["uniqname"].toString(),
                                            m["key"].toString()
                                            );
    }
    else
    {
        sipMessage = new TomahawkSipMessage();
    }


    qDebug() << "Send sip messsage to " << to;
    Jreen::IQ iq( Jreen::IQ::Set, to );
    iq.addExtension( sipMessage );

    m_client->send( iq, this, SLOT( onNewIq( Jreen::IQ, int ) ), SipMessageSent );
}

void
JabberPlugin::broadcastMsg(const QString& msg)
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_client )
        return;

    foreach( const Jreen::JID& jid, m_peers.keys() )
    {
        sendMsg( jid.full(), msg );
    }
}

void
JabberPlugin::addContact(const QString& jid, const QString& msg)
{
    // Add contact to the Tomahawk group on the roster
    m_roster->subscribe( jid, msg, jid, QStringList() << "Tomahawk" );

    return;
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

    if ( reconnect )
    {
        qDebug() << Q_FUNC_INFO << "Reconnecting jreen plugin...";
        disconnectPlugin();

        m_currentUsername = accountName();
        m_currentPassword = readPassword();
        m_currentServer = readServer();
        m_currentPort = readPort();

        setupClientHelper();

        qDebug() << Q_FUNC_INFO << "Updated settings";
        connectPlugin( false );
    }
}

void JabberPlugin::setupClientHelper()
{
    Jreen::JID jid = Jreen::JID( m_currentUsername );

    m_client->setJID( jid );
    m_client->setPassword( m_currentPassword );

    if( !m_currentServer.isEmpty() )
    {
        // set explicit server details
        m_client->setServer( m_currentServer );
        m_client->setPort( m_currentPort );
    }
    else
    {
        // let jreen find out server and port via jdns
        m_client->setServer( jid.domain() );
        m_client->setPort( -1 );
    }
}

void JabberPlugin::addMenuHelper()
{
    if( !m_menu )
    {
        m_menu = new QMenu( QString( "%1 (" ).arg( friendlyName() ).append( accountName() ).append(")" ) );
        m_addFriendAction = m_menu->addAction( "Add Friend..." );

        connect( m_addFriendAction, SIGNAL( triggered() ), this, SLOT( showAddFriendDialog() ) );

        emit addMenu( m_menu );
    }
}

void JabberPlugin::removeMenuHelper()
{
    if( m_menu && m_addFriendAction )
    {
        emit removeMenu( m_menu );

        delete m_menu;
        m_menu = 0;
        m_addFriendAction = 0; // deleted by menu
    }
}

void JabberPlugin::onNewMessage(const Jreen::Message& message)
{
    QString from = message.from().full();
    QString msg = message.body();

    SipInfo info = SipInfo::fromJson( msg );

    if ( !info.isValid() )
    {
        QString to = from;
        QString response = QString( tr("I'm sorry -- I'm just an automatic presence used by Tomahawk Player"
                                    " (http://gettomahawk.com). If you are getting this message, the person you"
                                    " are trying to reach is probably not signed on, so please try again later!") );

        // this is not a sip message, so we send it directly through the client
        m_client->send( Jreen::Message ( Jreen::Message::Chat, Jreen::JID(to), response) );

        emit msgReceived( from, msg );
        return;
    }

    qDebug() << Q_FUNC_INFO << "From:" << message.from().full() << ":" << message.body();
    emit sipInfoReceived( from, info );
}


void JabberPlugin::onPresenceReceived( const Jreen::RosterItem::Ptr &item, const Jreen::Presence& presence )
{
    Q_UNUSED(item);

    Jreen::JID jid = presence.from();
    QString fulljid( jid.full() );


    qDebug() << Q_FUNC_INFO << "* New presence: " << fulljid << presence.subtype();

    if( jid == m_client->jid() )
        return;

    if ( presence.error() ) {
        //qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk: no" << "presence error";
        return;
    }

    // ignore anyone not Running tomahawk:
    Jreen::Capabilities::Ptr caps = presence.findExtension<Jreen::Capabilities>();
    if ( caps && ( caps->node() == TOMAHAWK_CAP_NODE_NAME ) )
    {
        // must be a jreen resource, implementation in gloox was broken
        qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk: yes" << "caps " << caps->node();
        handlePeerStatus( jid, presence.subtype() );
    }
    else if( caps )
    {
        qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk: maybe" << "caps " << caps->node()
            << "requesting disco..";

        // request disco features
        QString node = caps->node() + '#' + caps->ver();

        Jreen::IQ iq( Jreen::IQ::Get, jid );
        iq.addExtension( new Jreen::Disco::Info( node ) );

        m_client->send( iq, this, SLOT( onNewIq( Jreen::IQ, int ) ), RequestDisco );
    }
    else if( !caps )
    {
        qDebug() << Q_FUNC_INFO << "Running tomahawk: no" << "no caps";
    }
}

void JabberPlugin::onSubscriptionReceived(const Jreen::RosterItem::Ptr& item, const Jreen::Presence& presence)
{
    qDebug() << Q_FUNC_INFO << "presence type: " << presence.subtype();
    if(item)
        qDebug() << Q_FUNC_INFO << presence.from().full() << "subs" << item->subscription() << "ask" << item->ask();
    else
        qDebug() << Q_FUNC_INFO << "item empty";

    // don't do anything if the contact is already subscribed to us
    if( presence.subtype() != Jreen::Presence::Subscribe ||
        (
            item && (item->subscription() == Jreen::RosterItem::From || item->subscription() == Jreen::RosterItem::Both)
        )
    )
    {
        return;
    }

    // check if the requester is already on the roster
    if(item &&
        (
            item->subscription() == Jreen::RosterItem::To ||
            ( item->subscription() == Jreen::RosterItem::None && !item->ask().isEmpty() )
        )
    )
    {
        qDebug() << Q_FUNC_INFO << presence.from().bare() << "already on the roster so we assume ack'ing subscription request is okay...";
        m_roster->allowSubscription(presence.from(), true);

        return;
    }

    qDebug() << Q_FUNC_INFO << presence.from().bare() << "open subscription request box";

    // preparing the confirm box for the user
    QMessageBox *confirmBox = new QMessageBox(
                                QMessageBox::Question,
                                tr( "Authorize User" ),
                                QString( tr( "Do you want to grant <b>%1</b> access to your Collection?" ) ).arg(presence.from().bare()),
                                QMessageBox::Yes | QMessageBox::No,
                                0
                              );

    // add confirmBox to m_subscriptionConfirmBoxes
    m_subscriptionConfirmBoxes.insert( presence.from(), confirmBox );

    // display the box and wait for the answer
    confirmBox->open( this, SLOT( onSubscriptionRequestConfirmed( int ) ) );
}

void
JabberPlugin::onSubscriptionRequestConfirmed( int result )
{
    qDebug() << Q_FUNC_INFO << result;

    QList< QMessageBox* > confirmBoxes = m_subscriptionConfirmBoxes.values();
    Jreen::JID jid;

    foreach( QMessageBox* currentBox, confirmBoxes )
    {
        if( currentBox == sender() )
        {
            jid = m_subscriptionConfirmBoxes.key( currentBox );
        }
    }

    qDebug() << Q_FUNC_INFO << "box confirmed for" << jid.bare();

    // we got an answer, deleting the box
    m_subscriptionConfirmBoxes.remove( jid );
    sender()->deleteLater();

    QMessageBox::StandardButton allowSubscription = static_cast<QMessageBox::StandardButton>( result );

    if ( allowSubscription == QMessageBox::Yes )
    {
        qDebug() << Q_FUNC_INFO << jid.bare() << "accepted by user, adding to roster";
        addContact(jid, "");
    }
    else
    {
        qDebug() << Q_FUNC_INFO << jid.bare() << "declined by user";
    }

    m_roster->allowSubscription( jid, allowSubscription == QMessageBox::Yes );
}

void JabberPlugin::onNewIq(const Jreen::IQ& iq, int context)
{
    if( context == RequestDisco )
    {
        qDebug() << Q_FUNC_INFO << "Received disco IQ...";
        Jreen::Disco::Info *discoInfo = iq.findExtension<Jreen::Disco::Info>().data();
        if(!discoInfo)
            return;
        iq.accept();

        Jreen::JID jid = iq.from();

        Jreen::DataForm::Ptr form = discoInfo->form();

        if(discoInfo->features().contains( TOMAHAWK_FEATURE ))
        {
            qDebug() << Q_FUNC_INFO << jid.full() << "Running tomahawk/feature enabled: yes";

            // the actual presence doesn't matter, it just needs to be "online"
            handlePeerStatus( jid, Jreen::Presence::Available );
        }
        else
        {
            qDebug() << Q_FUNC_INFO << jid.full() << "Running tomahawk/feature enabled: no";

            //LEGACY: accept resources starting with tomahawk too
            if( jid.resource().startsWith("tomahawk") )
            {
                qDebug() << Q_FUNC_INFO << jid.full() << "Detected legacy tomahawk..";

                // add to legacy peers, so we can send text messages instead of iqs
                m_legacy_peers.append( jid );

                handlePeerStatus( jid, Jreen::Presence::Available );
            }
        }
    }
    else if(context == RequestedDisco)
    {
        qDebug() << "Sent IQ(Set), what should be happening here?";
    }
    else if(context == SipMessageSent )
    {
        qDebug() << "Sent SipMessage... what now?!";
    }
    /*else if(context == RequestedVCard )
    {
        qDebug() << "Requested VCard... what now?!";
    }*/
    else
    {

        TomahawkSipMessage *sipMessage = iq.findExtension<TomahawkSipMessage>().data();
        if(sipMessage)
        {
            iq.accept();

            qDebug() << Q_FUNC_INFO << "Got SipMessage ...";
            qDebug() << Q_FUNC_INFO << "ip" << sipMessage->ip();
            qDebug() << Q_FUNC_INFO << "port" << sipMessage->port();
            qDebug() << Q_FUNC_INFO << "uniqname" << sipMessage->uniqname();
            qDebug() << Q_FUNC_INFO << "key" << sipMessage->key();
            qDebug() << Q_FUNC_INFO << "visible" << sipMessage->visible();

            SipInfo info;
            info.setVisible( sipMessage->visible() );
            if( sipMessage->visible() )
            {
                info.setHost( QHostAddress( sipMessage->ip() ) );
                info.setPort( sipMessage->port() );
                info.setUniqname( sipMessage->uniqname() );
                info.setKey( sipMessage->key() );
            }

            Q_ASSERT( info.isValid() );

            qDebug() << Q_FUNC_INFO << "From:" << iq.from().full() << ":" << info;
            emit sipInfoReceived( iq.from().full(), info );
        }
    }
}

bool JabberPlugin::presenceMeansOnline(Jreen::Presence::Type p)
{
    switch(p)
    {
        case Jreen::Presence::Invalid:
        case Jreen::Presence::Unavailable:
        case Jreen::Presence::Error:
            return false;
            break;
        default:
            return true;
    }
}

void JabberPlugin::handlePeerStatus(const Jreen::JID& jid, Jreen::Presence::Type presenceType)
{
    QString fulljid = jid.full();

    // "going offline" event
    if ( !presenceMeansOnline( presenceType ) &&
         ( !m_peers.contains( jid ) ||
           presenceMeansOnline( m_peers.value( jid ) )
         )
       )
    {
        m_peers[ jid ] = presenceType;
        qDebug() << Q_FUNC_INFO << "* Peer goes offline:" << fulljid;

        // remove peer from legacy peers
        if( m_legacy_peers.contains( jid ) )
        {
            m_legacy_peers.removeAll( jid );
        }

        emit peerOffline( fulljid );
        return;
    }

    // "coming online" event
    if( presenceMeansOnline( presenceType ) &&
        ( !m_peers.contains( jid ) ||
          !presenceMeansOnline( m_peers.value( jid ) )
        )
       )
    {
        m_peers[ jid ] = presenceType;
        qDebug() << Q_FUNC_INFO << "* Peer goes online:" << fulljid;

        emit peerOnline( fulljid );

        if(!m_avatarManager->avatar(jid.bare()).isNull())
            onNewAvatar( jid.bare() );

        return;
    }

    //qDebug() << "Updating presence data for" << fulljid;
    m_peers[ jid ] = presenceType;
}

void JabberPlugin::onNewAvatar(const QString& jid)
{
    qDebug() << Q_FUNC_INFO << jid;
    Q_ASSERT(!m_avatarManager->avatar( jid ).isNull());

    // find peers for the jid
    QList<Jreen::JID> peers =  m_peers.keys();
    foreach(const Jreen::JID &peer, peers)
    {
        if( peer.bare() == jid )
        {
            emit avatarReceived ( peer.full(),  m_avatarManager->avatar( jid ) );
        }
    }

    if( jid == m_client->jid().bare() )
        // own avatar
        emit avatarReceived ( m_avatarManager->avatar( jid ) );
    else
        // someone else's avatar
        emit avatarReceived ( jid,  m_avatarManager->avatar( jid ) );
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

void
JabberPlugin::saveConfig()
{
    TomahawkSettings::instance()->setValue( pluginId() + "/username", m_ui->jabberUsername->text() );
    TomahawkSettings::instance()->setValue( pluginId() + "/password", m_ui->jabberPassword->text() );
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
