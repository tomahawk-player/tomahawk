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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tomahawksipmessagefactory.h"

#include <QStringList>
#include <QXmlStreamWriter>
#include <QVariant>

#include "utils/logger.h"

using namespace Jreen;

TomahawkSipMessageFactory::TomahawkSipMessageFactory()
{
    m_depth = 0;
    m_state = AtNowhere;
}

TomahawkSipMessageFactory::~TomahawkSipMessageFactory()
{
}

QStringList TomahawkSipMessageFactory::features() const
{
    return QStringList(TOMAHAWK_SIP_MESSAGE_NS);
}

bool TomahawkSipMessageFactory::canParse(const QStringRef &name, const QStringRef &uri, const QXmlStreamAttributes &attributes)
{
    Q_UNUSED(uri);
    Q_UNUSED(attributes);
    return name == QLatin1String("tomahawk") && uri == TOMAHAWK_SIP_MESSAGE_NS;
}

void TomahawkSipMessageFactory::handleStartElement(const QStringRef &name, const QStringRef &uri,
                                            const QXmlStreamAttributes &attributes)
{
    m_depth++;
    if (m_depth == 1) {
        m_state = AtNowhere;
        m_ip = QString();
        m_port = -1;
        m_uniqname = QString();
        m_key = QString();
        m_visible = false;
    } else if (m_depth == 2) {
        if (name == QLatin1String("transport"))
        {
//            qDebug() << "Found Transport";
            m_state = AtTransport;

            m_uniqname = attributes.value(QLatin1String("uniqname")).toString();
            m_key = attributes.value(QLatin1String("pwd")).toString();
        }
    } else if(m_depth == 3) {
        if (name == QLatin1String("candidate"))
        {
            m_state = AtCandidate;
//            qDebug() << "Found candidate";
            m_ip = attributes.value(QLatin1String("ip")).toString();
            m_port = attributes.value(QLatin1String("port")).toString().toInt();

            m_visible = true;
        }
    }
    Q_UNUSED(uri);
    Q_UNUSED(attributes);
}

void TomahawkSipMessageFactory::handleEndElement(const QStringRef &name, const QStringRef &uri)
{
    if (m_depth == 3)
        m_state = AtNowhere;
    Q_UNUSED(name);
    Q_UNUSED(uri);
    m_depth--;
}

void TomahawkSipMessageFactory::handleCharacterData(const QStringRef &text)
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

void TomahawkSipMessageFactory::serialize(Payload *extension, QXmlStreamWriter *writer)
{
    TomahawkSipMessage *sipMessage = se_cast<TomahawkSipMessage*>(extension);

        writer->writeStartElement(QLatin1String("tomahawk"));
        writer->writeDefaultNamespace(TOMAHAWK_SIP_MESSAGE_NS);

        if(sipMessage->visible())
        {
            // add transport tag
            writer->writeStartElement(QLatin1String("transport"));
            writer->writeAttribute(QLatin1String("pwd"), sipMessage->key());
            writer->writeAttribute(QLatin1String("uniqname"), sipMessage->uniqname());

            writer->writeEmptyElement(QLatin1String("candidate"));
            writer->writeAttribute(QLatin1String("component"), "1");
            writer->writeAttribute(QLatin1String("id"), "el0747fg11"); // FIXME
            writer->writeAttribute(QLatin1String("ip"), sipMessage->ip());
            writer->writeAttribute(QLatin1String("network"), "1");
            writer->writeAttribute(QLatin1String("port"), QVariant(sipMessage->port()).toString());
            writer->writeAttribute(QLatin1String("priority"), "1"); //TODO
            writer->writeAttribute(QLatin1String("protocol"), "tcp");
            writer->writeAttribute(QLatin1String("type"), "host"); //FIXME: correct?!
            writer->writeEndElement();
        }
        else
        {
            writer->writeEmptyElement(QLatin1String("transport"));
        }
        writer->writeEndElement();
}

Payload::Ptr TomahawkSipMessageFactory::createPayload()
{
    if(m_visible)
        return Payload::Ptr(new TomahawkSipMessage(m_ip, m_port, m_uniqname, m_key));
    else
        return Payload::Ptr(new TomahawkSipMessage());
}
