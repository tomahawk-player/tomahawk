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

#ifndef ENTITYTIME_H
#define ENTITYTIME_H

#include <jreen/stanzaextension.h>

#include "sip/SipInfo.h"

#define TOMAHAWK_SIP_MESSAGE_NS QLatin1String("http://www.tomhawk-player.org/sip/transports")

#include "accounts/AccountDllMacro.h"

class ACCOUNTDLLEXPORT TomahawkXmppMessage : public Jreen::Payload
{
    J_PAYLOAD(TomahawkXmppMessage)
    public:
        TomahawkXmppMessage();
        TomahawkXmppMessage(const QList<SipInfo>& sipInfos);
        ~TomahawkXmppMessage();

        /**
         * The SipInfo objects that are wrapped in this XmppMessage
         */
        const QList<SipInfo> sipInfos() const;

        /**
         * The name of the peer contained in this message
         */
        const QString key() const;

        /**
         * The name of the peer contained in this message
         */
        const QString uniqname() const;

    private:
        QList<SipInfo> m_sipInfos;
};

#endif // ENTITYTIME_H
