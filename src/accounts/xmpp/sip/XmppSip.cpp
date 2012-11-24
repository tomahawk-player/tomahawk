/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "XmppSip.h"


#include "TomahawkXmppMessage.h"
#include "TomahawkXmppMessageFactory.h"

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "accounts/AccountManager.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtilsGui.h"

#include "config.h"
#include "TomahawkVersion.h"

#include <jreen/jid.h>
#include <jreen/capabilities.h>
#include <jreen/vcardupdate.h>
#include <jreen/vcard.h>
#include <jreen/directconnection.h>
#include <jreen/tcpconnection.h>
#include <jreen/softwareversion.h>
#include <jreen/iqreply.h>
#include <jreen/logger.h>
#include <jreen/tune.h>

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QtPlugin>
#include <QStringList>
#include <QDateTime>
#include <QTimer>

#ifndef ENABLE_HEADLESS
    #include <QInputDialog>
    #include <QLineEdit>
    #include <QMessageBox>
#endif

using namespace Tomahawk;
using namespace Accounts;

// instead of simply copying this function for another thirdparty lib
// please make it a meta-function or a macro and put it in Logger.h. kthxbbq
#define JREEN_LOG_INFIX "Jreen"
#define TOMAHAWK_FEATURE QLatin1String( "tomahawk:sip:v1" )
#define TOMAHAWK_CAP_NODE_NAME QLatin1String( "http://tomahawk-player.org/" )


void
JreenMessageHandler( QtMsgType type, const char *msg )
{
    switch ( type )
    {
        case QtDebugMsg:
            tDebug( LOGTHIRDPARTY ).nospace() << JREEN_LOG_INFIX << ":" << "Debug:" << msg;
            break;
        case QtWarningMsg:
            tDebug( LOGTHIRDPARTY ).nospace() << JREEN_LOG_INFIX << ":" << "Warning:" << msg;
            break;
        case QtCriticalMsg:
            tDebug( LOGTHIRDPARTY ).nospace() << JREEN_LOG_INFIX << ":" << "Critical:" << msg;
            break;
        case QtFatalMsg:
            tDebug( LOGTHIRDPARTY ).nospace() << JREEN_LOG_INFIX << ":" << "Fatal:" << msg;
            abort();
    }
}


XmppSipPlugin::XmppSipPlugin( Account* account )
    : SipPlugin( account )
    , m_state( Account::Disconnected )
#ifndef ENABLE_HEADLESS
    , m_menu( 0 )
    , m_xmlConsole( 0 )
    , m_pubSubManager( 0 )
#endif
{
    Jreen::Logger::addHandler( JreenMessageHandler );

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

#ifndef ENABLE_HEADLESS
    // instantiate XmlConsole
    if( readXmlConsoleEnabled() )
    {
        m_xmlConsole = new XmlConsole( m_client );
        m_xmlConsole->show();
    }
#endif

    // add VCardUpdate extension to own presence
    m_client->presence().addExtension( new Jreen::VCardUpdate() );

    // initaliaze the roster
    m_roster = new Jreen::SimpleRoster( m_client );

#ifndef ENABLE_HEADLESS
    // initialize the AvatarManager
    m_avatarManager = new AvatarManager( m_client );
#endif

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
    connect( m_client, SIGNAL( serverFeaturesReceived( QSet<QString> ) ), SLOT( onConnect() ) );
    connect( m_client, SIGNAL( disconnected( Jreen::Client::DisconnectReason ) ), SLOT( onDisconnect( Jreen::Client::DisconnectReason ) ) );
    connect( m_client, SIGNAL( messageReceived( Jreen::Message ) ), SLOT( onNewMessage( Jreen::Message ) ) );

    connect( m_client, SIGNAL( iqReceived( Jreen::IQ ) ), SLOT( onNewIq( Jreen::IQ ) ) );

    connect( m_roster, SIGNAL( presenceReceived( Jreen::RosterItem::Ptr, Jreen::Presence ) ),
                         SLOT( onPresenceReceived( Jreen::RosterItem::Ptr, Jreen::Presence ) ) );
    connect( m_roster, SIGNAL( subscriptionReceived( Jreen::RosterItem::Ptr, Jreen::Presence ) ),
                         SLOT( onSubscriptionReceived( Jreen::RosterItem::Ptr, Jreen::Presence ) ) );

#ifndef ENABLE_HEADLESS
    connect( m_avatarManager, SIGNAL( newAvatar( QString ) ), SLOT( onNewAvatar( QString ) ) );
#endif

    m_pubSubManager = new Jreen::PubSub::Manager( m_client );
    m_pubSubManager->addEntityType< Jreen::Tune >();

    // Clear status
    Jreen::Tune::Ptr tune( new Jreen::Tune() );
    m_pubSubManager->publishItems( QList<Jreen::Payload::Ptr>() << tune, Jreen::JID() );
}


XmppSipPlugin::~XmppSipPlugin()
{
    //Note: the next two lines don't currently work, because the deletion wipes out internally posted events, need to talk to euro about a fix
    Jreen::Tune::Ptr tune( new Jreen::Tune() );
    m_pubSubManager->publishItems( QList<Jreen::Payload::Ptr>() << tune, Jreen::JID() );

    delete m_pubSubManager;
    delete m_avatarManager;
    delete m_roster;
#ifndef ENABLE_HEADLESS
    delete m_xmlConsole;
#endif
    delete m_client;
}

QString
XmppSipPlugin::inviteString() const
{
    return tr( "Enter Jabber ID" );
}


InfoSystem::InfoPluginPtr
XmppSipPlugin::infoPlugin()
{
    if ( m_infoPlugin.isNull() )
        m_infoPlugin = QWeakPointer< Tomahawk::InfoSystem::XmppInfoPlugin >( new Tomahawk::InfoSystem::XmppInfoPlugin( this ) );

    return InfoSystem::InfoPluginPtr( m_infoPlugin.data() );
}


#ifndef ENABLE_HEADLESS
QMenu*
XmppSipPlugin::menu()
{
    return m_menu;
}
#endif


void
XmppSipPlugin::connectPlugin()
{
    if ( m_client->isConnected() )
    {
        qDebug() << Q_FUNC_INFO << "Already connected to server, not connecting again...";
        return;
    }

    if ( m_account->configuration().contains( "enforcesecure" ) && m_account->configuration().value( "enforcesecure" ).toBool() )
    {
        tLog() << Q_FUNC_INFO << "Enforcing secure connection...";
        m_client->setFeatureConfig( Jreen::Client::Encryption, Jreen::Client::Force );
    }

    tDebug() << "Connecting to the Xmpp server..." << m_client->jid().full();

    //FIXME: we're badly workarounding some missing reconnection api here, to be fixed soon
    QTimer::singleShot( 1000, m_client, SLOT( connectToServer() ) );

    if ( m_client->connection() )
        connect( m_client->connection(), SIGNAL( error( Jreen::Connection::SocketError ) ), SLOT( onError( Jreen::Connection::SocketError ) ) );

    m_state = Account::Connecting;
    emit stateChanged( m_state );
}


void
XmppSipPlugin::disconnectPlugin()
{
    if ( !m_client->isConnected() )
    {
        if ( m_state != Account::Disconnected ) // might be Connecting
        {
            m_state = Account::Disconnected;
            emit stateChanged( m_state );
        }
        return;
    }

    //m_roster->deleteLater();
    //m_roster = 0;
    //m_room->deleteLater();
    //m_room = 0;

    m_peers.clear();

    publishTune( QUrl(), Tomahawk::InfoSystem::InfoStringHash() );

    m_state = Account::Disconnecting;
    emit stateChanged( m_state );

    m_client->disconnectFromServer( true );
}


void
XmppSipPlugin::onConnect()
{
    // update jid resource, servers like gtalk use resource binding and may
    // have changed our requested /resource
    if ( m_client->jid().resource() != m_currentResource )
    {
        m_currentResource = m_client->jid().resource();
        emit jidChanged( m_client->jid().full() );
    }

    // set presence to least valid value
    m_client->setPresence( Jreen::Presence::XA, "Got Tomahawk? http://gettomahawk.com", -127 );
    m_client->setPingInterval( 1000 );
    m_roster->load();

    // load XmppInfoPlugin
    if ( infoPlugin() && Tomahawk::InfoSystem::InfoSystem::instance()->workerThread() )
    {
        infoPlugin().data()->moveToThread( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() );
        Tomahawk::InfoSystem::InfoSystem::instance()->addInfoPlugin( infoPlugin() );
    }

    //FIXME: this implementation is totally broken atm, so it's disabled to avoid harm :P
    // join MUC with bare jid as nickname
    //TODO: make the room a list of rooms and make that configurable
    // QString mucNickname = QString( "tomahawk@conference.qutim.org/" ).append( QString( m_client->jid().bare() ).replace( "@", "-" ) );
    //m_room = new Jreen::MUCRoom(m_client, Jreen::JID( mucNickname ) );
    //m_room->setHistorySeconds(0);
    //m_room->join();

    // treat muc participiants like contacts
    //connect( m_room, SIGNAL( messageReceived( Jreen::Message, bool ) ), this, SLOT( onNewMessage( Jreen::Message ) ) );
    //connect( m_room, SIGNAL( presenceReceived( Jreen::Presence, const Jreen::MUCRoom::Participant* ) ), this, SLOT( onNewPresence( Jreen::Presence ) ) );

    m_state = Account::Connected;
    emit stateChanged( m_state );

    addMenuHelper();
}


void
XmppSipPlugin::onDisconnect( Jreen::Client::DisconnectReason reason )
{
    switch( reason )
    {
        case Jreen::Client::User:
            break;

        case Jreen::Client::AuthorizationError:
            emit error( Account::AuthError, errorMessage( reason ) );
            break;

        case Jreen::Client::HostUnknown:
        case Jreen::Client::ItemNotFound:
        case Jreen::Client::RemoteStreamError:
        case Jreen::Client::RemoteConnectionFailed:
        case Jreen::Client::InternalServerError:
        case Jreen::Client::SystemShutdown:
        case Jreen::Client::Conflict:
        case Jreen::Client::Unknown:
        case Jreen::Client::NoCompressionSupport:
        case Jreen::Client::NoEncryptionSupport:
        case Jreen::Client::NoAuthorizationSupport:
        case Jreen::Client::NoSupportedFeature:
            emit error( Account::ConnectionError, errorMessage( reason ) );
            break;

        default:
            qDebug() << "Not all Client::DisconnectReasons checked" << ( int ) reason;
            Q_ASSERT( false );
            break;
    }
    m_state = Account::Disconnected;
    emit stateChanged( m_state );

    removeMenuHelper();

    Q_FOREACH( const Jreen::JID &peer, m_peers.keys() )
    {
        handlePeerStatus( peer, Jreen::Presence::Unavailable );
    }

    if ( !m_infoPlugin.isNull() )
        Tomahawk::InfoSystem::InfoSystem::instance()->removeInfoPlugin( infoPlugin() );
}


void
XmppSipPlugin::onError( const Jreen::Connection::SocketError& e )
{
    tDebug() << "JABBER error:" << e;
}


QString
XmppSipPlugin::errorMessage( Jreen::Client::DisconnectReason reason )
{
    switch( reason )
    {
        case Jreen::Client::User:
            return tr( "User Interaction" );
            break;
        case Jreen::Client::HostUnknown:
            return tr( "Host is unknown" );
            break;
        case Jreen::Client::ItemNotFound:
            return tr( "Item not found" );
            break;
        case Jreen::Client::AuthorizationError:
            return tr( "Authorization Error" );
            break;
        case Jreen::Client::RemoteStreamError:
            return tr( "Remote Stream Error" );
            break;
        case Jreen::Client::RemoteConnectionFailed:
            return tr( "Remote Connection failed" );
            break;
        case Jreen::Client::InternalServerError:
            return tr( "Internal Server Error" );
            break;
        case Jreen::Client::SystemShutdown:
            return tr( "System shutdown" );
            break;
        case Jreen::Client::Conflict:
            return tr( "Conflict" );
            break;
        case Jreen::Client::NoCompressionSupport:
            return tr( "No Compression Support" );
            break;
        case Jreen::Client::NoEncryptionSupport:
            return tr( "No Encryption Support" );
            break;
        case Jreen::Client::NoAuthorizationSupport:
            return tr( "No Authorization Support" );
            break;
        case Jreen::Client::NoSupportedFeature:
            return tr( "No Supported Feature" );
            break;
        case Jreen::Client::Unknown:
            return tr( "Unknown" );
            break;

        default:
            qDebug() << "Not all Client::DisconnectReasons checked";
            Q_ASSERT( false );
            break;
    }

    m_state = Account::Disconnected;
    emit stateChanged( m_state );

    return QString();
}


void
XmppSipPlugin::sendMsg( const QString& to, const SipInfo& info )
{
    qDebug() << Q_FUNC_INFO << to << info;

    if ( !m_client )
        return;

    TomahawkXmppMessage *sipMessage;
    if ( info.isVisible() )
    {
        sipMessage = new TomahawkXmppMessage( info.host(), info.port(), info.uniqname(), info.key() );
    }
    else
        sipMessage = new TomahawkXmppMessage();

    qDebug() << "Send sip messsage to" << to;
    Jreen::IQ iq( Jreen::IQ::Set, to );
    iq.addExtension( sipMessage );
    Jreen::IQReply *reply = m_client->send( iq );
    reply->setData( SipMessageSent );
    connect( reply, SIGNAL( received( Jreen::IQ ) ), SLOT( onNewIq( Jreen::IQ ) ) );
}


void
XmppSipPlugin::addContact( const QString& jid, const QString& msg )
{
    // Add contact to the Tomahawk group on the roster
    QString realJid = jid;
    if ( !realJid.contains( '@' ) )
        realJid += defaultSuffix();

    m_roster->subscribe( realJid, msg, realJid, QStringList() << "Tomahawk" );
}


void
XmppSipPlugin::showAddFriendDialog()
{
#ifndef ENABLE_HEADLESS
    bool ok;
    QString id = QInputDialog::getText( TomahawkUtils::tomahawkWindow(), tr( "Add Friend" ),
                                        tr( "Enter Xmpp ID:" ), QLineEdit::Normal, "", &ok ).trimmed();

    if ( !ok )
        return;

    qDebug() << "Attempting to add xmpp contact to roster:" << id;
    addContact( id );
#endif
}


void
XmppSipPlugin::publishTune( const QUrl& url, const InfoSystem::InfoStringHash& trackInfo )
{
    if ( m_account->configuration().value("publishtracks").toBool() == false )
    {
        tDebug() << Q_FUNC_INFO << m_client->jid().full() << "Not publishing now playing info (disabled in account config)";
        return;
    }

    if ( trackInfo.isEmpty() )
    {
        Jreen::Tune::Ptr tune( new Jreen::Tune() );
        m_pubSubManager->publishItems( QList<Jreen::Payload::Ptr>() << tune, Jreen::JID() );
    }

    Jreen::Tune::Ptr tune( new Jreen::Tune() );

    tune->setTitle( trackInfo.value( "title" ) );
    tune->setArtist( trackInfo.value( "artist" ) );
    tune->setLength( trackInfo.value( "duration" ).toInt() );
    tune->setTrack( trackInfo.value( "albumpos" ) );

    //TODO: provide a rating once available in Tomahawk
    tune->setRating( 10 );

    //TODO: it would be nice to set Spotify, Dilandau etc here, but not the jabber ids of friends
    tune->setSource( "Tomahawk" );

    tune->setUri( url );
    tDebug() << Q_FUNC_INFO << "Setting URI of" << tune->uri().toString();

    m_pubSubManager->publishItems( QList<Jreen::Payload::Ptr>() << tune, Jreen::JID() );
}


QString
XmppSipPlugin::defaultSuffix() const
{
    return "@xmpp.org";
}


void
XmppSipPlugin::showXmlConsole()
{
#ifndef ENABLE_HEADLESS
   m_xmlConsole->show();
#endif
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


void
XmppSipPlugin::setupClientHelper()
{
    m_client->setProxyFactory( TomahawkUtils::proxyFactory( true ) );
    Jreen::JID jid = Jreen::JID( m_currentUsername );
    m_client->setJID( jid );
    m_client->setPassword( m_currentPassword );

    if ( !m_currentServer.isEmpty() )
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


void
XmppSipPlugin::addMenuHelper()
{
#ifndef ENABLE_HEADLESS
    if ( !m_menu )
    {
        m_menu = new QMenu( QString( "%1 (" ).arg( friendlyName() ).append( readUsername() ).append(")" ) );

        QAction* addFriendAction = m_menu->addAction( tr( "Add Friend..." ) );
        connect( addFriendAction, SIGNAL( triggered() ), SLOT( showAddFriendDialog() ) );

        if ( readXmlConsoleEnabled() )
        {
            QAction* showXmlConsoleAction = m_menu->addAction( tr( "XML Console..." ) );
            connect( showXmlConsoleAction, SIGNAL( triggered() ), SLOT( showXmlConsole() ) );
        }

        emit addMenu( m_menu );
    }
#endif
}


void
XmppSipPlugin::removeMenuHelper()
{
#ifndef ENABLE_HEADLESS
    if ( m_menu )
    {
        emit removeMenu( m_menu );

        delete m_menu;
        m_menu = 0;
    }
#endif
}


void
XmppSipPlugin::onNewMessage( const Jreen::Message& message )
{
    if ( m_state != Account::Connected )
        return;

    QString from = message.from().full();
    QString msg = message.body();

    if ( msg.isEmpty() )
        return;

    if ( message.subtype() == Jreen::Message::Error )
    {
        tDebug() << Q_FUNC_INFO << "Received error message from" << from << ", not answering... (Condition:"
                 << ( message.error().isNull() ? -1 : message.error()->condition() ) << ")";
        return;
    }

    SipInfo info = SipInfo::fromJson( msg );
    if ( !info.isValid() )
    {
        QString to = from;
        QString response = QString( tr( "I'm sorry -- I'm just an automatic presence used by Tomahawk Player"
                                        " (http://gettomahawk.com). If you are getting this message, the person you"
                                        " are trying to reach is probably not signed on, so please try again later!" ) );

        // this is not a sip message, so we send it directly through the client
        m_client->send( Jreen::Message ( Jreen::Message::Error, Jreen::JID( to ), response) );

        emit msgReceived( from, msg );
        return;
    }

    qDebug() << Q_FUNC_INFO << "From:" << message.from().full() << ":" << message.body();
    emit sipInfoReceived( from, info );
}


void
XmppSipPlugin::onPresenceReceived( const Jreen::RosterItem::Ptr& item, const Jreen::Presence& presence )
{
    Q_UNUSED(item);
    if ( m_state != Account::Connected )
        return;

    Jreen::JID jid = presence.from();
    QString fulljid( jid.full() );

    qDebug() << Q_FUNC_INFO << "* New presence:" << fulljid << presence.subtype();

    if ( jid == m_client->jid() )
        return;

    if ( presence.error() )
    {
        //qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk: no" << "presence error";
        return;
    }

    // ignore anyone not Running tomahawk:
    Jreen::Capabilities::Ptr caps = presence.payload<Jreen::Capabilities>();
    if ( caps )
    {
        qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk: maybe" << "caps" << caps->node() << "requesting disco...";

        // request disco features
        QString node = caps->node() + '#' + caps->ver();

        Jreen::IQ featuresIq( Jreen::IQ::Get, jid );
        featuresIq.addExtension( new Jreen::Disco::Info( node ) );

        Jreen::IQReply *reply = m_client->send( featuresIq );
        reply->setData( RequestDisco );
        connect( reply, SIGNAL( received( Jreen::IQ ) ), SLOT( onNewIq( Jreen::IQ ) ) );
    }
    else if ( !caps )
    {
//        qDebug() << Q_FUNC_INFO << "Running tomahawk: no" << "no caps";
        if ( presenceMeansOnline( m_peers[ jid ] ) )
            handlePeerStatus( jid, Jreen::Presence::Unavailable );
    }
}


void
XmppSipPlugin::onSubscriptionReceived( const Jreen::RosterItem::Ptr& item, const Jreen::Presence& presence )
{
    if ( m_state != Account::Connected )
        return;

    if ( item )
        qDebug() << Q_FUNC_INFO << presence.from().full() << "subs" << item->subscription() << "ask" << item->ask();
    else
        qDebug() << Q_FUNC_INFO << "item empty";

    // don't do anything if the contact is already subscribed to us
    if ( presence.subtype() != Jreen::Presence::Subscribe ||
       ( item && (item->subscription() == Jreen::RosterItem::From || item->subscription() == Jreen::RosterItem::Both ) ) )
    {
        return;
    }

    // check if the requester is already on the roster
    if ( item &&
       ( item->subscription() == Jreen::RosterItem::To || ( item->subscription() == Jreen::RosterItem::None && !item->ask().isEmpty() ) ) )
    {
        qDebug() << Q_FUNC_INFO << presence.from().bare() << "already on the roster so we assume ack'ing subscription request is okay...";
        m_roster->allowSubscription( presence.from(), true );

        return;
    }

#ifndef ENABLE_HEADLESS
    // preparing the confirm box for the user
    QMessageBox *confirmBox = new QMessageBox(
                                QMessageBox::Question,
                                tr( "Authorize User" ),
                                QString( tr( "Do you want to add <b>%1</b> to your friend list?" ) ).arg( presence.from().bare() ),
                                QMessageBox::Yes | QMessageBox::No,
                                TomahawkUtils::tomahawkWindow()
                              );

    // add confirmBox to m_subscriptionConfirmBoxes
    m_subscriptionConfirmBoxes.insert( presence.from(), confirmBox );

    // display the box and wait for the answer
    confirmBox->open( this, SLOT( onSubscriptionRequestConfirmed( int ) ) );
#endif
}


void
XmppSipPlugin::onSubscriptionRequestConfirmed( int result )
{
#ifndef ENABLE_HEADLESS
    qDebug() << Q_FUNC_INFO << result;

    QList< QMessageBox* > confirmBoxes = m_subscriptionConfirmBoxes.values();
    Jreen::JID jid;

    foreach ( QMessageBox* currentBox, confirmBoxes )
    {
        if ( currentBox == sender() )
        {
            jid = m_subscriptionConfirmBoxes.key( currentBox );
        }
    }

    // we got an answer, deleting the box
    m_subscriptionConfirmBoxes.remove( jid );
    sender()->deleteLater();

    QMessageBox::StandardButton allowSubscription = static_cast< QMessageBox::StandardButton >( result );
    if ( allowSubscription == QMessageBox::Yes )
    {
        qDebug() << Q_FUNC_INFO << jid.bare() << "accepted by user, adding to roster";
        addContact( jid, "" );
    }
    else
    {
        qDebug() << Q_FUNC_INFO << jid.bare() << "declined by user";
    }

    m_roster->allowSubscription( jid, allowSubscription == QMessageBox::Yes );
#endif
}


void
XmppSipPlugin::onNewIq( const Jreen::IQ& iq )
{
    if ( m_state != Account::Connected )
        return;

    Jreen::IQReply *reply = qobject_cast< Jreen::IQReply* >( sender() );
    int context = reply ? reply->data().toInt() : NoContext;

    if ( context == RequestDisco )
    {
//        qDebug() << Q_FUNC_INFO << "Received disco IQ...";
        Jreen::Disco::Info *discoInfo = iq.payload< Jreen::Disco::Info >().data();
        if ( !discoInfo )
            return;

        iq.accept();
        Jreen::JID jid = iq.from();
        Jreen::DataForm::Ptr form = discoInfo->form();

        if ( discoInfo->features().contains( TOMAHAWK_FEATURE ) )
        {
            qDebug() << Q_FUNC_INFO << jid.full() << "Running tomahawk/feature enabled: yes";

            // the actual presence doesn't matter, it just needs to be "online"
            handlePeerStatus( jid, Jreen::Presence::Available );
        }
    }
    else if ( context == RequestVersion)
    {
        Jreen::SoftwareVersion::Ptr softwareVersion = iq.payload<Jreen::SoftwareVersion>();
        if ( softwareVersion )
        {
            QString versionString = QString( "%1 %2 %3" ).arg( softwareVersion->name(), softwareVersion->os(), softwareVersion->version() );
            qDebug() << Q_FUNC_INFO << "Received software version for" << iq.from().full() << ":" << versionString;
            emit softwareVersionReceived( iq.from().full(), versionString );
        }
    }
    else if ( context == RequestedDisco )
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
        TomahawkXmppMessage::Ptr sipMessage = iq.payload< TomahawkXmppMessage >();
        if(sipMessage)
        {
            iq.accept();

            qDebug() << Q_FUNC_INFO << "Got SipMessage ..."
                     << "ip" << sipMessage->ip() << "port" << sipMessage->port() << "uniqname" << sipMessage->uniqname() << "key" << sipMessage->key() << "visible" << sipMessage->visible();

            SipInfo info;
            info.setVisible( sipMessage->visible() );
            if ( sipMessage->visible() )
            {
                info.setHost( sipMessage->ip() );
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


bool
XmppSipPlugin::presenceMeansOnline( Jreen::Presence::Type p )
{
    switch( p )
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


void
XmppSipPlugin::handlePeerStatus( const Jreen::JID& jid, Jreen::Presence::Type presenceType )
{
    QString fulljid = jid.full();

    // "going offline" event
    if ( !presenceMeansOnline( presenceType ) &&
       ( !m_peers.contains( jid ) || presenceMeansOnline( m_peers.value( jid ) ) ) )
    {
        m_peers[ jid ] = presenceType;
        qDebug() << Q_FUNC_INFO << "* Peer goes offline:" << fulljid;

        emit peerOffline( fulljid );
        return;
    }

    // "coming online" event
    if ( presenceMeansOnline( presenceType ) &&
       ( !m_peers.contains( jid ) || !presenceMeansOnline( m_peers.value( jid ) ) ) )
    {
        m_peers[ jid ] = presenceType;
        qDebug() << Q_FUNC_INFO << "* Peer goes online:" << fulljid;

        emit peerOnline( fulljid );

#ifndef ENABLE_HEADLESS
        if ( !m_avatarManager->avatar( jid.bare() ).isNull() )
            onNewAvatar( jid.bare() );
#endif

        // request software version
        Jreen::IQ versionIq( Jreen::IQ::Get, jid );
        versionIq.addExtension( new Jreen::SoftwareVersion() );
        Jreen::IQReply *reply = m_client->send( versionIq );
        reply->setData( RequestVersion );
        connect( reply, SIGNAL( received( Jreen::IQ ) ), SLOT( onNewIq( Jreen::IQ ) ) );

        return;
    }

    //qDebug() << "Updating presence data for" << fulljid;
    m_peers[ jid ] = presenceType;
}


void
XmppSipPlugin::onNewAvatar( const QString& jid )
{
#ifndef ENABLE_HEADLESS
//    qDebug() << Q_FUNC_INFO << jid;
    if ( m_state != Account::Connected )
        return;

    Q_ASSERT( !m_avatarManager->avatar( jid ).isNull() );

    // find peers for the jid
    QList< Jreen::JID > peers =  m_peers.keys();
    foreach ( const Jreen::JID& peer, peers )
    {
        if ( peer.bare() == jid )
        {
            emit avatarReceived( peer.full(), m_avatarManager->avatar( jid ) );
        }
    }

    if ( jid == m_client->jid().bare() )
        // own avatar
        emit avatarReceived( m_avatarManager->avatar( jid ) );
    else
        // someone else's avatar
        emit avatarReceived( jid,  m_avatarManager->avatar( jid ) );
#endif
}


bool
XmppSipPlugin::readXmlConsoleEnabled()
{
    // HACK we want to allow xmlconsole to be set manually in the onfig file, which means we can't hide it in a QVariantHash
    const bool xmlConsole = TomahawkSettings::instance()->value( QString( "accounts/%1/xmlconsole" ).arg( account()->accountId() ), false ).toBool();
    return xmlConsole;
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


Account::ConnectionState
XmppSipPlugin::connectionState() const
{
    return m_state;
}
