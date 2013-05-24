/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "XmppBot.h"

#include "infosystem/InfoSystem.h"
#include "Album.h"
#include "Typedefs.h"
#include "TomahawkSettings.h"

#include "audio/AudioEngine.h"

#include <gloox/client.h>
#include <gloox/rostermanager.h>
#include <gloox/message.h>
#include <gloox/connectiontcpclient.h>

#include <QtCore/QStringList>

#include "utils/Logger.h"

using namespace gloox;
using namespace Tomahawk::InfoSystem;

static QString s_botInfoIdentifier = QString( "XMPPBot" );


XMPPBot::XMPPBot(QObject *parent)
    : QObject(parent)
    , m_currReturnMessage("\n")
{
    qDebug() << Q_FUNC_INFO;
    TomahawkSettings *settings = TomahawkSettings::instance();
    QString server = settings->xmppBotServer();
    QString jidstring = settings->xmppBotJid();
    QString password = settings->xmppBotPassword();
    int port = settings->xmppBotPort();
    if (jidstring.isEmpty() || password.isEmpty())
        return;

    JID jid(jidstring.toStdString());
    jid.setResource( QString( "tomahawkbot%1" ).arg( qrand() ).toStdString() );

    m_client = new XMPPBotClient(this, jid, password.toStdString(), port);
    if (!server.isEmpty())
        m_client.data()->setServer(server.toStdString());

    m_client.data()->registerConnectionListener(this);
    m_client.data()->registerSubscriptionHandler(this);
    m_client.data()->registerMessageHandler(this);
    m_client.data()->setPresence(Presence::Available, 1, "Tomahawkbot available");

    connect(AudioEngine::instance(), SIGNAL(started(const Tomahawk::result_ptr &)),
            SLOT(newTrackSlot(const Tomahawk::result_ptr &)));

    connect(InfoSystem::instance(),
        SIGNAL(info(QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, QVariantMap)),
        SLOT(infoReturnedSlot(QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, QVariantMap)));

    connect(InfoSystem::instance(), SIGNAL(finished(QString)), SLOT(infoFinishedSlot(QString)));

    bool success = m_client.data()->gloox::Client::connect(false);
    if (success)
        m_client.data()->run();
    else
        qDebug() << "XMPPBot failed to connect with Client";
}

XMPPBot::~XMPPBot()
{
    qDebug() << Q_FUNC_INFO;
    if (!m_client.isNull())
        m_client.data()->gloox::Client::disconnect();
}

void XMPPBot::newTrackSlot(const Tomahawk::result_ptr &track)
{
    m_currTrack = track;
    if (!track)
        return;
    QString status = QString("%1 - %2 (%3)")
                    .arg(track->artist()->name())
                    .arg(track->track())
                    .arg(track->album()->name());

    m_client.data()->setPresence(Presence::Available, 1, status.toStdString());
}

void XMPPBot::onConnect()
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "XMPPBot Connected";
}

void XMPPBot::onDisconnect(ConnectionError e)
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "XMPPBot Disconnected";
    qDebug() << "Connection error msg:" << e;
}

bool XMPPBot::onTLSConnect(const gloox::CertInfo& info)
{
    //WARNING: Blindly accepts all certificates, at the moment
    qDebug() << Q_FUNC_INFO;
    return true;
}

void XMPPBot::handleSubscription(const gloox::Subscription& subscription)
{
    qDebug() << Q_FUNC_INFO;
    if (subscription.subtype() == Subscription::Subscribed)
    {
        qDebug() << "XMPPBot is now subscribed to " << subscription.from().bare().c_str();
        return;
    }
    else if(subscription.subtype() == Subscription::Unsubscribed)
    {
        qDebug() << "XMPPBot is now unsubscribed from " << subscription.from().bare().c_str();
        return;
    }
    else if(subscription.subtype() == Subscription::Subscribe)
    {
        m_client.data()->rosterManager()->ackSubscriptionRequest(subscription.from().bareJID(), true);
        m_client.data()->rosterManager()->subscribe(subscription.from().bareJID(), EmptyString, StringList(), "Let me in?");
    }
    else if(subscription.subtype() == Subscription::Unsubscribe)
    {
        m_client.data()->rosterManager()->ackSubscriptionRequest(subscription.from().bareJID(), true);
        m_client.data()->rosterManager()->unsubscribe(subscription.from().bareJID(), "Sorry to see you go.");
    }
}

void XMPPBot::handleMessage(const Message& msg, MessageSession* session)
{
    //TODO: implement "properly" with MessageSessions, if the bot is to be multi-user
    if ( msg.subtype() != Message::Chat || msg.from().full().empty() || msg.to().full().empty() )
        return;

    QString body = QString::fromStdString( msg.body() ).toLower().trimmed();
    QString originatingJid = QString::fromStdString( msg.from().full() );

    if ( body.startsWith( "play" ) )
    {
        QStringList tokens = body.right( body.length() - 5 ).split( QString( "-" ), QString::SkipEmptyParts );
        if ( tokens.count() < 2 )
            AudioEngine::instance()->play();

        Tomahawk::query_ptr q = Tomahawk::Query::get( tokens.first().trimmed(), tokens.last().trimmed(), QString() );
        if ( q.isNull() )
            return;

        connect( q.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ),
                             SLOT( onResultsAdded( QList<Tomahawk::result_ptr> ) ) );

        return;
    }
    else if ( body.startsWith( "stop" ) )
    {
        AudioEngine::instance()->stop();
        return;
    }
    else if ( body.startsWith( "prev" ) )
    {
        AudioEngine::instance()->previous();
        return;
    }
    else if ( body.startsWith( "next" ) )
    {
        AudioEngine::instance()->next();
        return;
    }
    else if ( body.startsWith( "pause" ) )
    {
        AudioEngine::instance()->pause();
        return;
    }

    QStringList tokens( body.split( QString( " and " ), QString::SkipEmptyParts ) );
    if ( tokens.isEmpty() )
        return;

    qDebug() << "jid from:" << QString::fromStdString(msg.from().full()) << ", jid to:" << QString::fromStdString(msg.to().full());
    qDebug() << "Operating on tokens:" << tokens;

    if (m_currTrack.isNull() || m_currTrack->artist()->name().isEmpty() || m_currTrack->track().isEmpty())
    {
        qDebug() << "XMPPBot can't figure out track";
        QString m_currReturnMessage("\n\nSorry, I can't figure out what track is playing.\n\n");
        Message retMsg(Message::Chat, JID(originatingJid.toStdString()), m_currReturnMessage.toStdString());
        m_client.data()->send(retMsg);
        return;
    }

    InfoTypeMap infoMap;
    Q_FOREACH(QString token, tokens)
    {
        if (token == "biography")
            infoMap[InfoArtistBiography] = m_currTrack.data()->artist()->name();
        if (token == "terms")
            infoMap[InfoArtistTerms] = m_currTrack.data()->artist()->name();
        if (token == "hotttness")
            infoMap[InfoArtistHotttness] = m_currTrack.data()->artist()->name();
        if (token == "familiarity")
            infoMap[InfoArtistFamiliarity] = m_currTrack.data()->artist()->name();
        if (token == "lyrics")
        {
            QVariantMap myhash;
            myhash["trackName"] = QVariant::fromValue<QString>(m_currTrack.data()->track());
            myhash["artistName"] = QVariant::fromValue<QString>(m_currTrack.data()->artist()->name());
            infoMap[InfoTrackLyrics] = QVariant::fromValue<QVariantMap>(myhash);
        }
    }

    if (infoMap.isEmpty())
    {
        qDebug() << "XMPPBot can't figure out track";
        QString m_currReturnMessage("\n\nSorry, I couldn't recognize any commands.\n\n");
        Message retMsg(Message::Chat, JID(originatingJid.toStdString()), m_currReturnMessage.toStdString());
        m_client.data()->send(retMsg);
        return;
    }

    m_currInfoMap.unite(infoMap);
    QString waitMsg("Please wait...");
    Message retMsg(Message::Chat, JID(originatingJid.toStdString()), waitMsg.toStdString());
    m_client.data()->send(retMsg);
    QVariantMap hash;
    hash["XMPPBotSendToJID"] = originatingJid;
    InfoSystem::instance()->getInfo(s_botInfoIdentifier, infoMap, hash);
}

void XMPPBot::infoReturnedSlot(QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, QVariantMap customData)
{
    qDebug() << Q_FUNC_INFO;

    if (caller != s_botInfoIdentifier ||
        input.isNull() || !input.isValid() ||
        !customData.contains("XMPPBotSendToJID")
       )
    {
        qDebug() << "Not the right object, custom data is null, or don't have a set JID";
        return;
    }

    if (!m_currInfoMap.contains(type))
    {
        qDebug() << "not in currInfoMap";
        return;
    }
    else
        m_currInfoMap.remove(type);

    QString jid = customData["XMPPBotSendToJID"].toString();
    if (!m_currReturnJid.isEmpty() && m_currReturnJid != jid && !m_currReturnMessage.isEmpty())
    {
        gloox::Message msg(Message::Chat, JID(jid.toStdString()), m_currReturnMessage.toStdString());
        m_client.data()->send(msg);
        m_currReturnMessage = QString("\n");
    }
    m_currReturnJid = jid;

    switch(type)
    {
        case InfoArtistBiography:
        {
            qDebug() << "Artist bio requested";
            if (!output.canConvert<Tomahawk::InfoSystem::InfoGenericMap>() ||
                !input.canConvert<QString>()
               )
            {
                qDebug() << "Variants failed to be valid";
                break;
            }
            InfoGenericMap bmap = output.value<Tomahawk::InfoSystem::InfoGenericMap>();
            QString artist = input.toString();
            m_currReturnMessage += QString("\nBiographies for %1\n").arg(artist);
            Q_FOREACH(QString source, bmap.keys())
            {
                m_currReturnMessage += (bmap[source]["attribution"].isEmpty() ?
                        QString("From %1:\n").arg(bmap[source]["site"]) :
                        QString("From %1 at %2:\n").arg(bmap[source]["attribution"]).arg(bmap[source]["site"]));
                m_currReturnMessage += bmap[source]["text"] + QString("\n");
            }
            break;
        }
        case InfoArtistTerms:
        {
            qDebug() << "Artist terms requested";
            if (!output.canConvert<Tomahawk::InfoSystem::InfoGenericMap>() ||
                !input.canConvert<QString>()
               )
            {
                qDebug() << "Variants failed to be valid";
                break;
            }
            InfoGenericMap tmap = output.value<Tomahawk::InfoSystem::InfoGenericMap>();
            QString artist = input.toString();
            m_currReturnMessage += tr("\nTerms for %1:\n").arg(artist);
            if (tmap.isEmpty())
                m_currReturnMessage += tr("No terms found, sorry.");
            else
            {
                bool first = true;
                Q_FOREACH(QString term, tmap.keys())
                {
                    if (first)
                        m_currReturnMessage += (first ?
                            QString("%1 (weight %2, frequency %3)")
                                .arg(term).arg(tmap[term]["weight"]).arg(tmap[term]["frequency"])
                              :
                            QString("\n%1 (weight %2, frequency %3)")
                                .arg(term).arg(tmap[term]["weight"]).arg(tmap[term]["frequency"])
                              );
                    first = false;
                }
                m_currReturnMessage += QString("\n");
            }
            break;
        }
        case InfoArtistHotttness:
        {
            qDebug() << "Artist hotttness requested";
            if (!output.canConvert<qreal>() ||
                !input.canConvert<QString>()
               )
            {
                qDebug() << "Variants failed to be valid";
                break;
            }
            QString artist = input.toString();
            qreal retVal = output.toReal();
            QString retValString = (retVal == 0.0 ? "(none)" : QString::number(retVal));
            m_currReturnMessage += tr("\nHotttness for %1: %2\n").arg(artist, retValString);
            break;
        }
        case InfoArtistFamiliarity:
        {
            qDebug() << "Artist familiarity requested";
            if (!output.canConvert<qreal>() ||
                !input.canConvert<QString>()
               )
            {
                qDebug() << "Variants failed to be valid";
                break;
            }
            QString artist = input.toString();
            qreal retVal = output.toReal();
            QString retValString = (retVal == 0.0 ? "(none)" : QString::number(retVal));
            m_currReturnMessage += tr("\nFamiliarity for %1: %2\n").arg(artist, retValString);
            break;
        }
        case InfoTrackLyrics:
        {
            qDebug() << "Lyrics requested";
            if (!output.canConvert<QString>() ||
                !input.canConvert<QVariantMap>()
               )
            {
                qDebug() << "Variants failed to be valid";
                break;
            }
            QVariantMap inHash = input.value<QVariantMap>();
            QString artist = inHash["artistName"].toString();
            QString track = inHash["trackName"].toString();
            QString lyrics = output.toString();
            qDebug() << "lyrics = " << lyrics;
            m_currReturnMessage += tr("\nLyrics for \"%1\" by %2:\n\n%3\n").arg(track, artist, lyrics);
            break;
        }
        default:
            break;
    }

    if (m_currReturnMessage.isEmpty())
    {
        qDebug() << "Empty message, not sending anything back";
        return;
    }

    qDebug() << "Going to send message: " << m_currReturnMessage << " to " << jid;

    //gloox::Message msg(Message::Chat, JID(jid.toStdString()), m_currReturnMessage.toStdString());
    //m_client.data()->send(msg);
}

void XMPPBot::infoFinishedSlot(QString caller)
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "current return message is" << m_currReturnMessage;
    qDebug() << "id is" << caller << "and our id is" << s_botInfoIdentifier;
    if (m_currReturnMessage.isEmpty() || caller != s_botInfoIdentifier)
        return;

    qDebug() << "Sending message to JID" << m_currReturnJid;
    gloox::Message msg(Message::Chat, JID(m_currReturnJid.toStdString()), m_currReturnMessage.toStdString());
    m_client.data()->send(msg);
    m_currReturnMessage = QString("\n");
    m_currReturnJid.clear();
}


void XMPPBot::onResultsAdded( const QList<Tomahawk::result_ptr>& result )
{
    AudioEngine::instance()->playItem( 0, result.first() );
}

///////////////////////////////////////////////////////////////////////////////////////

XMPPBotClient::XMPPBotClient(QObject *parent, JID &jid, std::string password, int port)
    : QObject(parent)
    , Client(jid, password, port)
    , m_timer(this)
{
    qDebug() << Q_FUNC_INFO;
}

XMPPBotClient::~XMPPBotClient()
{
    qDebug() << Q_FUNC_INFO;
}

void XMPPBotClient::run()
{
    qDebug() << Q_FUNC_INFO;
    QObject::connect(&m_timer, SIGNAL(timeout()), SLOT(recvSlot()));
    m_timer.start(200);
    qDebug() << "XMPPBot running";
}

void XMPPBotClient::recvSlot()
{
    gloox::ConnectionError error = recv(100);
    if (error != gloox::ConnNoError)
        qDebug() << "ERROR: in XMPPBotClient::recvSlot" << error;
}
