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

#ifndef ENTITYTIMEFACTORY_P_H
#define ENTITYTIMEFACTORY_P_H

#include "TomahawkXmppMessage.h"

#include <jreen/stanzaextension.h>

#include "accounts/AccountDllMacro.h"

class ACCOUNTDLLEXPORT TomahawkXmppMessageFactory : public Jreen::PayloadFactory<TomahawkXmppMessage>
{
public:
    TomahawkXmppMessageFactory();
    virtual ~TomahawkXmppMessageFactory();
    QStringList features() const;
    bool canParse(const QStringRef &name, const QStringRef &uri, const QXmlStreamAttributes &attributes);
    void handleStartElement(const QStringRef &name, const QStringRef &uri, const QXmlStreamAttributes &attributes);
    void handleEndElement(const QStringRef &name, const QStringRef &uri);
    void handleCharacterData(const QStringRef &text);
    void serialize(Jreen::Payload *extension, QXmlStreamWriter *writer);
    Jreen::Payload::Ptr createPayload();
private:
    void serializeSipInfo(SipInfo& info, QXmlStreamWriter *writer);

    enum State { AtNowhere, AtTransport, AtCandidate } m_state;

    /**
     * All the provided Sip informations
     */
    QList<SipInfo> m_sipInfos;

    /**
     * The current parsing depth
     */
    int m_depth;

    /**
     * The unique name of the peer
     */
    QString m_uniqname;

    /**
     * The authentication key of the peer
     */
    QString m_key;
};

#endif // ENTITYTIMEFACTORY_P_H
