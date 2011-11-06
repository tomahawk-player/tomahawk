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

#include "xmppsip.h"

#include "xmlconsole.h"

#include "config.h"

#include "tomahawkxmppmessage.h"
#include "tomahawkxmppmessagefactory.h"

#include <jreen/jid.h>
#include <jreen/capabilities.h>
#include <jreen/vcardupdate.h>
#include <jreen/vcard.h>
#include <jreen/directconnection.h>
#include <jreen/tcpconnection.h>
#include <jreen/softwareversion.h>
#include <jreen/iqreply.h>

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QtPlugin>
#include <QStringList>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QDateTime>
#include <QTimer>

#include <utils/tomahawkutils.h>
#include <utils/logger.h>
#include <accounts/accountmanager.h>

XmppSipPlugin::XmppSipPlugin( Tomahawk::Accounts::Account *account )
    : SipPlugin( account )
    , m_menu( 0 )
    , m_xmlConsole( 0 )
    , m_state( Disconnected )
{
    qDebug() << Q_FUNC_INFO;

    m_currentUsername = readUsername();
    m_currentServer = readServer();
    m_currentPassword = readPassword();
    m_currentPort = readPort();

    // setup JID object
    Jreen::JID jid = Jreen::JID( readUsername() );

    // general client setup
    m_client = new Jreen::Client( jid, m_currentPassword );
    setupClientHelper();

    m_client->registerPayload( new TomahawkXmppMessageFactory );
    m_currentResource = QString::fromAscii( "tomahawk%1" ).arg( QString::number( qrand() % 10000 ) );
    m_client->setResource( m_currentResource );

    // instantiate XmlConsole
    if( readXmlConsoleEnabled() )
    {
        m_xmlConsole = new XmlConsole( m_client );
        m_xmlConsole->show();
    }

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

    // setup caps node
    Jreen::Capabilities::Ptr caps = m_client->presence().payload<Jreen::Capabilities>();
    caps->setNode( TOMAHAWK_CAP_NODE_NAME );

    // print connection parameters
    qDebug() << "Our JID set to:" << m_client->jid().full();
    qDebug() << "Our Server set to:" << m_client->server();
    qDebug() << "Our Port set to" << m_client->port();

    // setup slots
    connect(m_client, SIGNAL(serverFeaturesReceived(QSet<QString>)), SLOT(onConnect()));
    connect(m_client, SIGNAL(disconnected(Jreen::Client::DisconnectReason)), SLOT(onDisconnect(Jreen::Client::DisconnectReason)));
    connect(m_client, SIGNAL(messageReceived(Jreen::Message)), SLOT(onNewMessage(Jreen::Message)));

    connect(m_client, SIGNAL(iqReceived(Jreen::IQ)), SLOT(onNewIq(Jreen::IQ)));

    connect(m_roster, SIGNAL(presenceReceived(Jreen::RosterItem::Ptr,Jreen::Presence)),
                      SLOT(onPresenceReceived(Jreen::RosterItem::Ptr,Jreen::Presence)));
    connect(m_roster, SIGNAL(subscriptionReceived(Jreen::RosterItem::Ptr,Jreen::Presence)),
                      SLOT(onSubscriptionReceived(Jreen::RosterItem::Ptr,Jreen::Presence)));

    connect(m_avatarManager, SIGNAL(newAvatar(QString)), SLOT(onNewAvatar(QString)));
}

XmppSipPlugin::~XmppSipPlugin()
{
    delete m_avatarManager;
    delete m_roster;
    delete m_xmlConsole;
    delete m_client;
}

QMenu*
XmppSipPlugin::menu()
{
    return m_menu;
}

void
XmppSipPlugin::connectPlugin()
{
    qDebug() << Q_FUNC_INFO;

    if( m_client->isConnected() )
    {
        qDebug() << Q_FUNC_INFO << "Already connected to server, not connecting again...";
        return; //FIXME: should i return false here?!
    }

    qDebug() << "Connecting to the Xmpp server..." << m_client->jid().full();

    //FIXME: we're badly workarounding some missing reconnection api here, to be fixed soon
    QTimer::singleShot( 1000, m_client, SLOT( connectToServer() ) );

    if ( m_client->connection() )
        connect(m_client->connection(), SIGNAL(error(SocketError)), SLOT(onError(SocketError)));

    m_state = Connecting;
    emit stateChanged( m_state );
    return;
}

void
XmppSipPlugin::disconnectPlugin()
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

    m_client->disconnectFromServer( true );
    m_state = Disconnecting;
    emit stateChanged( m_state );
}

void
XmppSipPlugin::onConnect()
{
//    qDebug() << Q_FUNC_INFO;

    // update jid resource, servers like gtalk use resource binding and may
    // have changed our requested /resource
    if ( m_client->jid().resource() != m_currentResource )
    {
        m_currentResource = m_client->jid().resource();
        emit jidChanged( m_client->jid().full() );
    }

    qDebug() << "Connected to xmpp as:" << m_client->jid().full();

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
XmppSipPlugin::onDisconnect( Jreen::Client::DisconnectReason reason )
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

void
XmppSipPlugin::onError( const Jreen::Connection::SocketError& e )
{
    tLog() << "JABBER error:" << e;
}

QString
XmppSipPlugin::errorMessage( Jreen::Client::DisconnectReason reason )
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
XmppSipPlugin::sendMsg(const QString& to, const QString& msg)
{
    qDebug() << Q_FUNC_INFO << to << msg;

    if ( !m_client ) {
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
        qDebug() << "Invalid JSON in Xmpp msg";
        return;
    }
    QVariantMap m = v.toMap();
    /*******************************************************/

    TomahawkXmppMessage *sipMessage;
    if(m["visible"].toBool())
    {
        sipMessage = new TomahawkXmppMessage(m["ip"].toString(),
                                            m["port"].toInt(),
                                            m["uniqname"].toString(),
                                            m["key"].toString()
                                            );
    }
    else
    {
        sipMessage = new TomahawkXmppMessage();
    }

    qDebug() << "Send sip messsage to " << to;
    Jreen::IQ iq( Jreen::IQ::Set, to );
    iq.addExtension( sipMessage );
    Jreen::IQReply *reply = m_client->send(iq);
    reply->setData(SipMessageSent);
    connect(reply, SIGNAL(received(Jreen::IQ)), SLOT(onNewIq(Jreen::IQ)));
}

void
XmppSipPlugin::broadcastMsg(const QString& msg)
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
XmppSipPlugin::addContact(const QString& jid, const QString& msg)
{
    // Add contact to the Tomahawk group on the roster

    QString realJid = jid;
    if( !realJid.contains( '@' ) )
        realJid += defaultSuffix();

    m_roster->subscribe( realJid, msg, realJid, QStringList() << "Tomahawk" );

    return;
}

void
XmppSipPlugin::showAddFriendDialog()
{
    bool ok;
    QString id = QInputDialog::getText( TomahawkUtils::tomahawkWindow(), tr( "Add Friend" ),
                                        tr( "Enter Xmpp ID:" ), QLineEdit::Normal, "", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to add xmpp contact to roster:" << id;
    addContact( id );
}

QString
XmppSipPlugin::defaultSuffix() const
{
    return "@xmpp.org";
}


void
XmppSipPlugin::showXmlConsole()
{
   m_xmlConsole->show();
}


void
XmppSipPlugin::checkSettings()
{
    configurationChanged();
}


void
XmppSipPlugin::configurationChanged()
{
    bool reconnect = false;

    QString username, password, server;
    int port;

    username = readUsername();
    password = readPassword();
    server = readServer();
    port = readPort();

    if ( m_currentUsername != username )
    {
        m_currentUsername = username;
        reconnect = true;
    }
    if ( m_currentPassword != password )
    {
        m_currentPassword = password;
        reconnect = true;
    }
    if ( m_currentServer != server )
    {
        m_currentServer = server;
        reconnect = true;
    }
    if ( m_currentPort != readPort() )
    {
        m_currentPort = port;
        reconnect = true;
    }

    if ( !m_currentUsername.contains( '@' ) )
    {
        m_currentUsername += defaultSuffix();
        QVariantHash credentials = m_account->credentials();
        credentials[ "username" ] = m_currentUsername;
        m_account->setCredentials( credentials );
        m_account->sync();
    }

    if ( reconnect )
    {
        qDebug() << Q_FUNC_INFO << "Reconnecting jreen plugin...";
        disconnectPlugin();

        setupClientHelper();

        qDebug() << Q_FUNC_INFO << "Updated settings";
        connectPlugin();
    }
}

void XmppSipPlugin::setupClientHelper()
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

void XmppSipPlugin::addMenuHelper()
{
    if( !m_menu )
    {
        m_menu = new QMenu( QString( "%1 (" ).arg( friendlyName() ).append( readUsername() ).append(")" ) );

        QAction* addFriendAction = m_menu->addAction( tr( "Add Friend..." ) );
        connect( addFriendAction, SIGNAL( triggered() ), this, SLOT( showAddFriendDialog() ) );

        if( readXmlConsoleEnabled() )
        {
            QAction* showXmlConsoleAction = m_menu->addAction( tr( "XML Console...") );
            connect( showXmlConsoleAction, SIGNAL( triggered() ), this, SLOT( showXmlConsole() ) );
        }

        emit addMenu( m_menu );
    }
}

void XmppSipPlugin::removeMenuHelper()
{
    if( m_menu )
    {
        emit removeMenu( m_menu );

        delete m_menu;
        m_menu = 0;
    }
}

void XmppSipPlugin::onNewMessage(const Jreen::Message& message)
{
    if ( m_state != Connected )
        return;

//    qDebug() << Q_FUNC_INFO << "message type" << message.subtype();

    QString from = message.from().full();
    QString msg = message.body();

    if(msg.isEmpty())
        return;

    if( message.subtype() == Jreen::Message::Error )
    {
        qDebug() << Q_FUNC_INFO << "Received error message from " << from << ", not answering... (Condition: "
                 << ( message.error().isNull() ? -1 : message.error()->condition() ) << ")";
        return;
    }

    SipInfo info = SipInfo::fromJson( msg );

    if ( !info.isValid() )
    {
        QString to = from;
        QString response = QString( tr("I'm sorry -- I'm just an automatic presence used by Tomahawk Player"
                                    " (http://gettomahawk.com). If you are getting this message, the person you"
                                    " are trying to reach is probably not signed on, so please try again later!") );

        // this is not a sip message, so we send it directly through the client
        m_client->send( Jreen::Message ( Jreen::Message::Error, Jreen::JID(to), response) );

        emit msgReceived( from, msg );
        return;
    }

    qDebug() << Q_FUNC_INFO << "From:" << message.from().full() << ":" << message.body();
    emit sipInfoReceived( from, info );
}


void XmppSipPlugin::onPresenceReceived( const Jreen::RosterItem::Ptr &item, const Jreen::Presence& presence )
{
    Q_UNUSED(item);
    if ( m_state != Connected )
        return;

    Jreen::JID jid = presence.from();
    QString fulljid( jid.full() );

//    qDebug() << Q_FUNC_INFO << "* New presence:" << fulljid << presence.subtype();

    if( jid == m_client->jid() )
        return;

    if ( presence.error() ) {
        //qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk: no" << "presence error";
        return;
    }

    // ignore anyone not Running tomahawk:
    Jreen::Capabilities::Ptr caps = presence.payload<Jreen::Capabilities>();
    if( caps )
    {
        qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk: maybe" << "caps " << caps->node() << "requesting disco...";

        // request disco features
        QString node = caps->node() + '#' + caps->ver();

        Jreen::IQ featuresIq( Jreen::IQ::Get, jid );
        featuresIq.addExtension( new Jreen::Disco::Info( node ) );

        Jreen::IQReply *reply = m_client->send(featuresIq);
        reply->setData(RequestDisco);
        connect(reply, SIGNAL(received(Jreen::IQ)), SLOT(onNewIq(Jreen::IQ)));
    }
    else if( !caps )
    {
//        qDebug() << Q_FUNC_INFO << "Running tomahawk: no" << "no caps";
        if ( presenceMeansOnline( m_peers[ jid ] ) )
            handlePeerStatus( jid, Jreen::Presence::Unavailable );
    }
}

void XmppSipPlugin::onSubscriptionReceived(const Jreen::RosterItem::Ptr& item, const Jreen::Presence& presence)
{
    if ( m_state != Connected )
        return;

//    qDebug() << Q_FUNC_INFO << "presence type:" << presence.subtype();
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

    // preparing the confirm box for the user
    QMessageBox *confirmBox = new QMessageBox(
                                QMessageBox::Question,
                                tr( "Authorize User" ),
                                QString( tr( "Do you want to grant <b>%1</b> access to your Collection?" ) ).arg(presence.from().bare()),
                                QMessageBox::Yes | QMessageBox::No,
                                TomahawkUtils::tomahawkWindow()
                              );

    // add confirmBox to m_subscriptionConfirmBoxes
    m_subscriptionConfirmBoxes.insert( presence.from(), confirmBox );

    // display the box and wait for the answer
    confirmBox->open( this, SLOT( onSubscriptionRequestConfirmed( int ) ) );
}

void
XmppSipPlugin::onSubscriptionRequestConfirmed( int result )
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

void XmppSipPlugin::onNewIq(const Jreen::IQ& iq)
{
    if ( m_state != Connected )
        return;

    Jreen::IQReply *reply = qobject_cast<Jreen::IQReply*>(sender());
    int context = reply ? reply->data().toInt() : NoContext;

    if( context == RequestDisco )
    {
//        qDebug() << Q_FUNC_INFO << "Received disco IQ...";
        Jreen::Disco::Info *discoInfo = iq.payload<Jreen::Disco::Info>().data();
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
    }
    else if(context == RequestVersion)
    {
        Jreen::SoftwareVersion::Ptr softwareVersion = iq.payload<Jreen::SoftwareVersion>();
        if( softwareVersion )
        {
            QString versionString = QString("%1 %2 %3").arg( softwareVersion->name(), softwareVersion->os(), softwareVersion->version() );
            qDebug() << Q_FUNC_INFO << "Received software version for " << iq.from().full() << ":" << versionString;
            emit softwareVersionReceived( iq.from().full(), versionString );
        }
    }
    else if(context == RequestedDisco)
    {
        qDebug() << "Sent IQ(Set), what should be happening here?";
    }
    else if( context == SipMessageSent )
    {
        qDebug() << "Sent SipMessage... what now?!";
    }
    /*else if(context == RequestedVCard )
    {
        qDebug() << "Requested VCard... what now?!";
    }*/
    else
    {
        TomahawkXmppMessage::Ptr sipMessage = iq.payload<TomahawkXmppMessage>();
        if(sipMessage)
        {
            iq.accept();

            qDebug() << Q_FUNC_INFO << "Got SipMessage ..."
                     << "ip" << sipMessage->ip() << "port" << sipMessage->port() << "uniqname" << sipMessage->uniqname() << "key" << sipMessage->key() << "visible" << sipMessage->visible();

            SipInfo info;
            info.setVisible( sipMessage->visible() );
            if( sipMessage->visible() )
            {

                QHostInfo hi;
                hi.setHostName( sipMessage->ip() );
                info.setHost( hi );
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

bool XmppSipPlugin::presenceMeansOnline(Jreen::Presence::Type p)
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

void XmppSipPlugin::handlePeerStatus(const Jreen::JID& jid, Jreen::Presence::Type presenceType)
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

        // request software version
        Jreen::IQ versionIq( Jreen::IQ::Get, jid );
        versionIq.addExtension( new Jreen::SoftwareVersion() );
        Jreen::IQReply *reply = m_client->send(versionIq);
        reply->setData(RequestVersion);
        connect(reply, SIGNAL(received(Jreen::IQ)), SLOT(onNewIq(Jreen::IQ)));

        return;
    }

    //qDebug() << "Updating presence data for" << fulljid;
    m_peers[ jid ] = presenceType;
}

void XmppSipPlugin::onNewAvatar(const QString& jid)
{
//    qDebug() << Q_FUNC_INFO << jid;
    if ( m_state != Connected )
        return;

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


bool
XmppSipPlugin::readXmlConsoleEnabled()
{
    QVariantHash configuration = m_account->configuration();
    return configuration.contains( "xmlconsole" ) && configuration[ "xmlconsole" ].toBool();
}


QString
XmppSipPlugin::readUsername()
{
    QVariantHash credentials = m_account->credentials();
    return credentials.contains( "username" ) ? credentials[ "username" ].toString() : QString();
}


QString
XmppSipPlugin::readPassword()
{
    QVariantHash credentials = m_account->credentials();
    return credentials.contains( "password" ) ? credentials[ "password" ].toString() : QString();
}


int
XmppSipPlugin::readPort()
{
    QVariantHash configuration = m_account->configuration();
    return configuration.contains( "port" ) ? configuration[ "port" ].toInt() : 5222;
}


QString
XmppSipPlugin::readServer()
{
    QVariantHash configuration = m_account->configuration();
    return configuration.contains( "server" ) ? configuration[ "server" ].toString() : QString();
}


SipPlugin::ConnectionState
XmppSipPlugin::connectionState() const
{
    return m_state;
}
