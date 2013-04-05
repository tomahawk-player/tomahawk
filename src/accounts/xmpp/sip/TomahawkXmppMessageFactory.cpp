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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TomahawkXmppMessageFactory.h"

#include "network/Servent.h"
#include "utils/Logger.h"

#include <QStringList>
#include <QXmlStreamWriter>
#include <QVariant>


using namespace Jreen;

TomahawkXmppMessageFactory::TomahawkXmppMessageFactory() : m_sipInfo()
{
    m_depth = 0;
    m_state = AtNowhere;
}

TomahawkXmppMessageFactory::~TomahawkXmppMessageFactory()
{
}

QStringList TomahawkXmppMessageFactory::features() const
{
    return QStringList(TOMAHAWK_SIP_MESSAGE_NS);
}

bool TomahawkXmppMessageFactory::canParse(const QStringRef &name, const QStringRef &uri, const QXmlStreamAttributes &attributes)
{
    Q_UNUSED(uri);
    Q_UNUSED(attributes);
    return name == QLatin1String("tomahawk") && uri == TOMAHAWK_SIP_MESSAGE_NS;
}

void TomahawkXmppMessageFactory::handleStartElement(const QStringRef &name, const QStringRef &uri,
                                            const QXmlStreamAttributes &attributes)
{
    m_depth++;
    if ( m_depth == 1 )
    {
        m_state = AtNowhere;
        m_uniqname = QString();
        m_key = QString();
        m_sipInfo = QList<SipInfo>();
    }
    else if ( m_depth == 2 )
    {
        if ( name == QLatin1String( "transport" ) )
        {
            m_state = AtTransport;
            m_uniqname = attributes.value( QLatin1String( "uniqname" ) ).toString();
            m_key = attributes.value( QLatin1String( "pwd" ) ).toString();
        }
    }
    else if(m_depth == 3)
    {
        if ( name == QLatin1String( "candidate" ) )
        {
            m_state = AtCandidate;
            SipInfo info = SipInfo();
            info.setVisible( true );
            info.setHost( attributes.value( QLatin1String( "ip" ) ).toString() );
            info.setPort( attributes.value( QLatin1String( "port" ) ).toString().toInt() );
            info.setKey( m_key );
            info.setNodeId( m_uniqname );
            Q_ASSERT( info.isValid() );
            m_sipInfo.append( info );
        }
    }
    Q_UNUSED(uri);
    Q_UNUSED(attributes);
}

void TomahawkXmppMessageFactory::handleEndElement(const QStringRef &name, const QStringRef &uri)
{
    if ( m_depth == 3 )
        m_state = AtTransport;
    else if ( m_depth == 2 )
    {
        m_state = AtNowhere;
        // Check that we have at least one SipInfo so that we provide some information about invisible peers.
        if ( m_sipInfo.length() == 0 )
        {
            SipInfo info = SipInfo();
            info.setVisible( false );
            info.setKey( m_key );
            info.setNodeId( m_uniqname );
            Q_ASSERT( info.isValid() );
            m_sipInfo.append( info );
        }
    }
    Q_UNUSED(name);
    Q_UNUSED(uri);
    m_depth--;
}

void TomahawkXmppMessageFactory::handleCharacterData(const QStringRef &text)
{
    /*if (m_state == AtUtc) {
        //m_utc = Util::fromStamp(text.toString());
    } else if (m_state == AtTzo) {
        QString str = text.toString();
        int multiple = str.startsWith('-') ? -1 : 1;
        //QTime delta = QTime::fromString(str.mid(1), QLatin1String("hh:mm"));
        //m_tzo = multiple * (delta.hour() * 60 + delta.minute());
    }*/
    Q_UNUSED(text);
}

void TomahawkXmppMessageFactory::serialize(Payload *extension, QXmlStreamWriter *writer)
{
    TomahawkXmppMessage *sipMessage = se_cast<TomahawkXmppMessage*>(extension);

    writer->writeStartElement( QLatin1String( "tomahawk" ) );
    writer->writeDefaultNamespace( TOMAHAWK_SIP_MESSAGE_NS );

    // Get a copy of the list, so that we can modify it here.
    QList<SipInfo> sipInfo = QList<SipInfo>( sipMessage->sipInfo() );
    QSharedPointer<SipInfo> lastInfo = QSharedPointer<SipInfo>();
    foreach ( SipInfo info, sipInfo )
    {
        if ( info.isVisible() )
        {
            QHostAddress ha = QHostAddress( info.host() );
            if ( Servent::isValidExternalIP( ha ) && ha.protocol() == QAbstractSocket::IPv4Protocol )
            {
                // For comapability reasons, this shall be put as the last candidate
                lastInfo = QSharedPointer<SipInfo>( new SipInfo( info ) );
                sipInfo.removeOne( info );
                break;
            }
        }
    }

    writer->writeStartElement( QLatin1String( "transport" ) );
    writer->writeAttribute( QLatin1String( "pwd" ), sipMessage->key() );
    writer->writeAttribute( QLatin1String( "uniqname" ), sipMessage->uniqname() );

    foreach ( SipInfo info, sipInfo )
    {
        if ( info.isVisible() )
            serializeSipInfo( info, writer );
    }

    if ( !lastInfo.isNull() )
    {
        Q_ASSERT( lastInfo->isVisible() );
        serializeSipInfo( *lastInfo, writer );
    }

    // </transport>
    writer->writeEndElement();
}

Payload::Ptr
TomahawkXmppMessageFactory::createPayload()
{
    if ( ( ( m_sipInfo.length() == 1 ) && ( !m_sipInfo.first().isVisible() ) ) || ( m_sipInfo.length() < 1 ) )
        return Payload::Ptr( new TomahawkXmppMessage() );
    else
        return Payload::Ptr( new TomahawkXmppMessage( m_sipInfo ) );
}

void
TomahawkXmppMessageFactory::serializeSipInfo(SipInfo &info, QXmlStreamWriter *writer)
{
    if ( info.isVisible() )
    {
        writer->writeEmptyElement( QLatin1String( "candidate" ) );
        writer->writeAttribute( QLatin1String( "component" ), "1" );
        writer->writeAttribute( QLatin1String( "id" ), "el0747fg11" ); // FIXME
        writer->writeAttribute( QLatin1String( "ip" ), info.host() );
        writer->writeAttribute( QLatin1String( "network" ), "1" );
        writer->writeAttribute( QLatin1String( "port" ), QVariant( info.port() ).toString() );
        writer->writeAttribute( QLatin1String( "priority" ), "1" ); //TODO
        writer->writeAttribute( QLatin1String( "protocol" ), "tcp" );
        writer->writeAttribute( QLatin1String( "type" ), "host" ); //FIXME: correct?!
    }
}
