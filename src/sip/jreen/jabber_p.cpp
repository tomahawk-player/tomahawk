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

#include "jabber_p.h"

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QString>
#include <QRegExp>
#include <QThread>
#include <utils/tomahawkutils.h>

#include <jreen/abstractroster.h>
#include <jreen/capabilities.h>


//remove
#include <QMessageBox>
#include <jreen/connection.h>

using namespace std;


#define TOMAHAWK_CAP_NODE_NAME QLatin1String("http://tomahawk-player.org/")

Jabber_p::Jabber_p( const QString& jid, const QString& password, const QString& server, const int port )
    : QObject()
    , m_server()
{
    qDebug() << Q_FUNC_INFO;
    //qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );
    qsrand(QDateTime::currentDateTime().toTime_t());

    m_presences[Jreen::Presence::Available] = "available";
    m_presences[Jreen::Presence::Chat] = "chat";
    m_presences[Jreen::Presence::Away] = "away";
    m_presences[Jreen::Presence::DND] = "dnd";
    m_presences[Jreen::Presence::XA] = "xa";
    m_presences[Jreen::Presence::Unavailable] = "unavailable";
    m_presences[Jreen::Presence::Probe] = "probe";
    m_presences[Jreen::Presence::Error] = "error";
    m_presences[Jreen::Presence::Invalid] = "invalid";

    m_jid = Jreen::JID( jid );

    m_client = new Jreen::Client( jid, password );
    m_client->disco()->setSoftwareVersion( "Tomahawk JREEN", "0.0.0.0", "Foobar" );

    m_client->disco()->addIdentity( Jreen::Disco::Identity( "client", "type", "tomahawk", "en" ) );
    m_client->disco()->addFeature( "tomahawk" );
    m_client->setResource( QString( "tomahawk%1" ).arg( "DOMME" ) );

    Jreen::Capabilities::Ptr caps = m_client->presence().findExtension<Jreen::Capabilities>();
    caps->setNode(TOMAHAWK_CAP_NODE_NAME);

    qDebug() << "Our JID set to:" << m_client->jid().full();
    qDebug() << "Our Server set to:" << m_client->server();
    qDebug() << "Our Port set to" << m_client->port();

    connect(m_client->connection(), SIGNAL(error(SocketError)), SLOT(onError(SocketError)));
    connect(m_client, SIGNAL(serverFeaturesReceived(QSet<QString>)), SLOT(onConnect()));
    connect(m_client, SIGNAL(disconnected(Jreen::Client::DisconnectReason)), SLOT(onDisconnect(Jreen::Client::DisconnectReason)));
    connect(m_client, SIGNAL(destroyed(QObject*)), this, SLOT(onDestroy()));
    connect(m_client, SIGNAL(newMessage(Jreen::Message)), SLOT(onNewMessage(Jreen::Message)));
    connect(m_client, SIGNAL(newPresence(Jreen::Presence)), SLOT(onNewPresence(Jreen::Presence)));


    qDebug() << "DISCOFEATURES:" << m_client->disco()->features();
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
    qDebug() << Q_FUNC_INFO;
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << Q_FUNC_INFO << "invoking in correct thread, not"
                 << QThread::currentThread();

        QMetaObject::invokeMethod( this, "sendMsg",
                                   Qt::QueuedConnection,
                                   Q_ARG( const QString, to ),
                                   Q_ARG( const QString, msg )
                                 );
        return;
    }

    if ( !m_client ) {
        return;
    }

    qDebug() << Q_FUNC_INFO << to << msg;
    Jreen::Message m( Jreen::Message::Chat, Jreen::JID(to), msg);

    m_client->send( m ); // assuming this is threadsafe
}


void
Jabber_p::broadcastMsg( const QString &msg )
{
    qDebug() << Q_FUNC_INFO;
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "broadcastMsg",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, msg)
                                 );
        return;
    }

    if ( !m_client )
        return;

    foreach( const QString& jidstr, m_peers.keys() )
    {
        qDebug() << "Broadcasting to" << jidstr <<"...";
        Jreen::Message m(Jreen::Message::Chat, Jreen::JID(jidstr), msg, "");
        m_client->send( m );
    }
}


void
Jabber_p::addContact( const QString& jid, const QString& msg )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "addContact",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, jid),
                                   Q_ARG(const QString, msg)
                                 );
        return;
    }

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
        m_jid.setResource( m_client->jid().resource() );
        QString jidstr( m_jid.full() );
        emit jidChanged( jidstr );
    }

    emit connected();
    qDebug() << "Connected as:" << m_jid.full();

    m_client->setPresence(Jreen::Presence::Available, "Tomahawk-JREEN available", 1);

    qDebug() << "DISCOFEATURES connected:" << m_client->disco()->features();
    m_client->setPingInterval(60000);

    m_roster = new Jreen::SimpleRoster( m_client );
    m_roster->load();

    // join MUC with bare jid as nickname
    //TODO: make the room a list of rooms and make that configurable
    QString bare(m_jid.bare());
    m_room = new Jreen::MUCRoom(m_client, Jreen::JID(QString("tomahawk@conference.qutim.org/").append(bare.replace("@", "-"))));
    //m_room->setHistorySeconds(0);
    //m_room->join();

    // treat muc participiants like contacts
    connect(m_room, SIGNAL(messageReceived(Jreen::Message, bool)), this, SLOT(onNewMessage(Jreen::Message)));
    connect(m_room, SIGNAL(presenceReceived(Jreen::Presence,const Jreen::MUCRoom::Participant*)), this, SLOT(onNewPresence(Jreen::Presence)));
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

    qDebug() << Q_FUNC_INFO << m.from().full() << ":" << m.body();
    emit msgReceived( from, msg );
}


void Jabber_p::onNewPresence( const Jreen::Presence& presence)
{

    Jreen::JID jid = presence.from();
    QString fulljid( jid.full() );

    qDebug() << Q_FUNC_INFO << "handle presence" << fulljid << presence.subtype();

    Jreen::IQ iq(Jreen::IQ::Get,jid);

    Jreen::Capabilities::Ptr caps = presence.findExtension<Jreen::Capabilities>();
    if(caps)
    {
        QString node = caps->node() + '#' + caps->ver();
        iq.addExtension(new Jreen::Disco::Info(node));
        m_client->send(iq,this,SLOT(onIQ(Jreen::IQ,int)),RequestDisco);
    }

    if( jid == m_jid )
        return;

    if ( presence.error() ) {
        qDebug() << Q_FUNC_INFO << "presence error: no tomahawk";
        return;
    }

    // ignore anyone not running tomahawk:
    //Jreen::Capabilities::Ptr caps = presence.findExtension<Jreen::Capabilities>();
    if ( caps && (caps->node() == TOMAHAWK_CAP_NODE_NAME ))
    {
        qDebug() << Q_FUNC_INFO << presence.from().full() << "tomahawk detected by caps";
    }
    // this is a hack actually as long as gloox based libsip_jabber is around
    // remove this as soon as everyone is using jreen
    else if( presence.from().resource().startsWith( QLatin1String("tomahawk") ) )
    {
        qDebug() << Q_FUNC_INFO << presence.from().full() << "tomahawk detected by resource";
    }
    else if( caps && caps->node() != TOMAHAWK_CAP_NODE_NAME )
    {
        qDebug() << Q_FUNC_INFO << presence.from().full() << "*no tomahawk* detected by caps!" << caps->node() << presence.from().resource();
        return;
    }
    else if( !caps )
    {
        qDebug() << Q_FUNC_INFO << "no tomahawk detected by resource and !caps";
        return;
    }

    qDebug() << Q_FUNC_INFO << fulljid << " is a tomahawk resource.";

    // "going offline" event
    if ( !presenceMeansOnline( presence.subtype() ) &&
         ( !m_peers.contains( fulljid ) ||
           presenceMeansOnline( m_peers.value( fulljid ) )
         )
       )
    {
        m_peers[ fulljid ] = presence.subtype();
        qDebug() << Q_FUNC_INFO << "* Peer goes offline:" << fulljid;
        emit peerOffline( fulljid );
        return;
    }

    // "coming online" event
    if( presenceMeansOnline( presence.subtype() ) &&
        ( !m_peers.contains( fulljid ) ||
          !presenceMeansOnline( m_peers.value( fulljid ) )
        )
       )
    {
        m_peers[ fulljid ] = presence.subtype();
        qDebug() << Q_FUNC_INFO << "* Peer goes online:" << fulljid;
        emit peerOnline( fulljid );
        return;
    }

    //qDebug() << "Updating presence data for" << fulljid;
    m_peers[ fulljid ] = presence.subtype();

}

void
Jabber_p::onIQ( const Jreen::IQ &iq, int context )
{
    if(context == RequestDisco) {
        Jreen::Disco::Info *discoInfo = iq.findExtension<Jreen::Disco::Info>().data();
        if(!discoInfo)
            return;
        iq.accept();


        QString jid = iq.from().full();
        Jreen::DataForm::Ptr form = discoInfo->form();

        qDebug() << jid << "NODE" << discoInfo->node();
        qDebug() << "jid:" << jid << "FEATURES" << discoInfo->features();
        qDebug() << jid << "DATA" << form;
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
