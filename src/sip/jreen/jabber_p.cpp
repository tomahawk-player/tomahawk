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

#include "jabber_p.h"
#include "tomahawksipmessage.h"
#include "tomahawksipmessagefactory.h"

#include "config.h"
#include "utils/tomahawkutils.h"

#include <jreen/capabilities.h>
#include <jreen/vcardupdate.h>
#include <jreen/vcard.h>

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QString>
#include <QRegExp>
#include <QThread>
#include <QVariant>
#include <QMap>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QPixmap>

//remove
#include <QLabel>
#include <QtGui/QLabel>

#define TOMAHAWK_FEATURE QLatin1String( "tomahawk:sip:v1" )

#define TOMAHAWK_CAP_NODE_NAME QLatin1String( "http://tomahawk-player.org/" )

Jabber_p::Jabber_p( const QString& jid, const QString& password, const QString& server, const int port )
    : QObject()
    , m_server()
{
    qDebug() << Q_FUNC_INFO;
    qsrand(QDateTime::currentDateTime().toTime_t());

    // setup JID object
    m_jid = Jreen::JID( jid );

    // general client setup
    m_client = new Jreen::Client( jid, password );
    m_client->registerStanzaExtension(new TomahawkSipMessageFactory);
    m_client->setResource( QString( "tomahawk%1" ).arg( QString::number( qrand() % 10000 ) ) );

    // add VCardUpdate extension to own presence
    m_client->presence().addExtension( new Jreen::VCardUpdate() );

    // read cached avatars
    m_photoHashes = QDir("/home/domme/jreen/").entryList();

    // setup disco
    m_client->disco()->setSoftwareVersion( "Tomahawk Player", TOMAHAWK_VERSION, CMAKE_SYSTEM );
    m_client->disco()->addIdentity( Jreen::Disco::Identity( "client", "type", "tomahawk", "en" ) );
    m_client->disco()->addFeature( TOMAHAWK_FEATURE );

    // setup caps node, legacy peer detection - used before 0.1
    Jreen::Capabilities::Ptr caps = m_client->presence().findExtension<Jreen::Capabilities>();
    caps->setNode( TOMAHAWK_CAP_NODE_NAME );

    // print connection parameters
    qDebug() << "Our JID set to:" << m_client->jid().full();
    qDebug() << "Our Server set to:" << m_client->server();
    qDebug() << "Our Port set to" << m_client->port();

    // setup slots
    connect(m_client->connection(), SIGNAL(error(SocketError)), SLOT(onError(SocketError)));
    connect(m_client, SIGNAL(serverFeaturesReceived(QSet<QString>)), SLOT(onConnect()));
    connect(m_client, SIGNAL(disconnected(Jreen::Client::DisconnectReason)), SLOT(onDisconnect(Jreen::Client::DisconnectReason)));
    connect(m_client, SIGNAL(destroyed(QObject*)), this, SLOT(onDestroy()));
    connect(m_client, SIGNAL(newMessage(Jreen::Message)), SLOT(onNewMessage(Jreen::Message)));
    connect(m_client, SIGNAL(newPresence(Jreen::Presence)), SLOT(onNewPresence(Jreen::Presence)));
    connect(m_client, SIGNAL(newIQ(Jreen::IQ)), SLOT(onNewIq(Jreen::IQ)));


    // connect
    qDebug() << "Connecting to the XMPP server...";
    m_client->connectToServer();
}


Jabber_p::~Jabber_p()
{
    delete m_client;
}

void
Jabber_p::setProxy( QNetworkProxy* proxy )
{
    qDebug() << Q_FUNC_INFO << "NOT IMPLEMENTED";
}

void
Jabber_p::disconnect()
{
    if ( m_client )
    {
        m_client->disconnect();
    }
}


void
Jabber_p::sendMsg( const QString& to, const QString& msg )
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
                                            m["key"].toString(),
                                            m["visible"].toBool()
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
Jabber_p::broadcastMsg( const QString &msg )
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_client )
        return;

    foreach( const QString& jidstr, m_peers.keys() )
    {
        sendMsg( jidstr, msg );
    }
}


void
Jabber_p::addContact( const QString& jid, const QString& msg )
{
    // Add contact to the Tomahawk group on the roster
    m_roster->add( jid, jid, QStringList() << "Tomahawk" );

    return;
}

void
Jabber_p::onConnect()
{
    qDebug() << Q_FUNC_INFO;

    // update jid resource, servers like gtalk use resource binding and may
    // have changed our requested /resource
    if ( m_client->jid().resource() != m_jid.resource() )
    {
        // TODO: check if this is still neccessary with jreen
        m_jid.setResource( m_client->jid().resource() );
        QString jidstr( m_jid.full() );
        emit jidChanged( jidstr );
    }

    emit connected();
    qDebug() << "Connected as:" << m_jid.full();

    // set presence to least valid value
    m_client->setPresence(Jreen::Presence::XA, "Got Tomahawk? http://gettomahawk.com", -127);

    // request own vcard
    fetchVCard( m_jid.bare() );

    // set ping timeout to 15 secs (TODO: verify if this works)
    m_client->setPingInterval(15000);

    // load roster
    m_roster = new Jreen::SimpleRoster( m_client );
    m_roster->load();

    //FIXME: this implementation is totally broken atm, so it's disabled to avoid harm :P
    // join MUC with bare jid as nickname
    //TODO: make the room a list of rooms and make that configurable
    QString mucNickname = QString( "tomahawk@conference.qutim.org/" ).append( QString( m_jid.bare() ).replace( "@", "-" ) );
    m_room = new Jreen::MUCRoom(m_client, Jreen::JID( mucNickname ) );
    //m_room->setHistorySeconds(0);
    //m_room->join();

    // treat muc participiants like contacts
    connect( m_room, SIGNAL( messageReceived( Jreen::Message, bool ) ), this, SLOT( onNewMessage( Jreen::Message ) ) );
    connect( m_room, SIGNAL( presenceReceived( Jreen::Presence, const Jreen::MUCRoom::Participant* ) ), this, SLOT( onNewPresence( Jreen::Presence ) ) );
}


void
Jabber_p::onDisconnect( Jreen::Client::DisconnectReason reason )
{
    QString error;
    bool reconnect = false;
    int reconnectInSeconds = 0;

    switch( reason )
    {
        case Jreen::Client::User:
            error = "User Interaction";
            break;
        case Jreen::Client::HostUnknown:
            error = "Host is unknown";
            break;
        case Jreen::Client::ItemNotFound:
            error = "Item not found";
            break;
        case Jreen::Client::AuthorizationError:
            error = "Authorization Error";
            break;
        case Jreen::Client::RemoteStreamError:
            error = "Remote Stream Error";
            reconnect = true;
            break;
        case Jreen::Client::RemoteConnectionFailed:
            error = "Remote Connection failed";
            break;
        case Jreen::Client::InternalServerError:
            error = "Internal Server Error";
            reconnect = true;
            break;
        case Jreen::Client::SystemShutdown:
            error = "System shutdown";
            reconnect = true;
            reconnectInSeconds = 60;
            break;
        case Jreen::Client::Conflict:
            error = "Conflict";
            break;

        case Jreen::Client::Unknown:
            error = "Unknown";
            break;

        default:
            qDebug() << "Not all Client::DisconnectReasons checked";
            Q_ASSERT(false);
            break;
    }

    qDebug() << "Disconnected from server:" << error;
    if( reason != Jreen::Client::User )
    {
        emit authError( reason, error );
    }

    if(reconnect)
        QTimer::singleShot(reconnectInSeconds*1000, m_client, SLOT(connectToServer()));

    emit disconnected();
}

void
Jabber_p::onNewMessage( const Jreen::Message& m )
{
    QString from = m.from().full();
    QString msg = m.body();

    if ( msg.isEmpty() )
        return;

    QJson::Parser parser;
    bool ok;
    QVariant v = parser.parse( msg.toAscii(), &ok );
    if ( !ok  || v.type() != QVariant::Map )
    {
        QString to = from;
        QString response = QString( tr("I'm sorry -- I'm just an automatic presence used by Tomahawk Player"
                                    " (http://gettomahawk.com). If you are getting this message, the person you"
                                    " are trying to reach is probably not signed on, so please try again later!") );

        // this is not a sip message, so we send it directly through the client
        m_client->send( Jreen::Message ( Jreen::Message::Chat, Jreen::JID(to), response) );

        return;
    }

    qDebug() << Q_FUNC_INFO << "From:" << m.from().full() << ":" << m.body();
    emit msgReceived( from, msg );
}


void Jabber_p::onNewPresence( const Jreen::Presence& presence)
{
    Jreen::JID jid = presence.from();
    QString fulljid( jid.full() );


    qDebug() << Q_FUNC_INFO << "* New presence: " << fulljid << presence.subtype();

    Jreen::VCardUpdate::Ptr update = presence.findExtension<Jreen::VCardUpdate>();
    if(update)
    {
        qDebug() << "vcard: found update for " << fulljid;
        if(!m_photoHashes.contains(update->photoHash()))
        {
            fetchVCard( jid.bare() );
        }
    }

    if( jid == m_jid )
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
        handlePeerStatus( fulljid, presence.subtype() );
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

void
Jabber_p::onNewIq( const Jreen::IQ &iq, int context )
{
    if( context == RequestDisco )
    {
        qDebug() << Q_FUNC_INFO << "Received disco IQ...";
        Jreen::Disco::Info *discoInfo = iq.findExtension<Jreen::Disco::Info>().data();
        if(!discoInfo)
            return;
        iq.accept();

        QString fulljid = iq.from().full();
        Jreen::DataForm::Ptr form = discoInfo->form();

        if(discoInfo->features().contains( TOMAHAWK_FEATURE ))
        {
            qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk/feature enabled: yes";

            // the actual presence doesn't matter, it just needs to be "online"
            handlePeerStatus( fulljid, Jreen::Presence::Available );
        }
        else
        {
            qDebug() << Q_FUNC_INFO << fulljid << "Running tomahawk/feature enabled: no";

            //LEGACY: accept resources starting with tomahawk too
            if( iq.from().resource().startsWith("tomahawk") )
            {
                qDebug() << Q_FUNC_INFO << fulljid << "Detected legacy tomahawk..";

                // add to legacy peers, so we can send text messages instead of iqs
                m_legacy_peers.append( fulljid );

                handlePeerStatus( fulljid, Jreen::Presence::Available );
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
            qDebug() << "ip" << sipMessage->ip();
            qDebug() << "port" << sipMessage->port();
            qDebug() << "uniqname" << sipMessage->uniqname();
            qDebug() << "key" << sipMessage->key();
            qDebug() << "visible" << sipMessage->visible();


            QVariantMap m;
            if( sipMessage->visible() )
            {
                m["visible"] = true;
                m["ip"] = sipMessage->ip();
                m["port"] = sipMessage->port();
                m["key"] = sipMessage->key();
                m["uniqname"] = sipMessage->uniqname();
            }
            else
            {
                m["visible"] = false;
            }


            QJson::Serializer ser;
            QByteArray ba = ser.serialize( m );
            QString msg = QString::fromAscii( ba );

            QString from = iq.from().full();
            qDebug() << Q_FUNC_INFO << "From:" << from << ":" << msg;
            emit msgReceived( from, msg );
        }
    }
}

bool
Jabber_p::presenceMeansOnline( Jreen::Presence::Type p )
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

void
Jabber_p::handlePeerStatus( const Jreen::JID& jid, Jreen::Presence::Type presenceType )
{
    QString fulljid = jid.full();

    // "going offline" event
    if ( !presenceMeansOnline( presenceType ) &&
         ( !m_peers.contains( fulljid ) ||
           presenceMeansOnline( m_peers.value( fulljid ) )
         )
       )
    {
        m_peers[ fulljid ] = presenceType;
        qDebug() << Q_FUNC_INFO << "* Peer goes offline:" << fulljid;

        // remove peer from legacy peers
        if( m_legacy_peers.contains( fulljid ) )
        {
            m_legacy_peers.removeAll( fulljid );
        }

        emit peerOffline( fulljid );
        return;
    }

    // "coming online" event
    if( presenceMeansOnline( presenceType ) &&
        ( !m_peers.contains( fulljid ) ||
          !presenceMeansOnline( m_peers.value( fulljid ) )
        )
       )
    {
        m_peers[ fulljid ] = presenceType;
        qDebug() << Q_FUNC_INFO << "* Peer goes online:" << fulljid;
        emit peerOnline( fulljid );
        return;
    }

    //qDebug() << "Updating presence data for" << fulljid;
    m_peers[ fulljid ] = presenceType;
}
