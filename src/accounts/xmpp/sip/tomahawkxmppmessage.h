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

#define TOMAHAWK_SIP_MESSAGE_NS QLatin1String("http://www.tomhawk-player.org/sip/transports")

#include "accounts/AccountDllMacro.h"

class TomahawkXmppMessagePrivate;
class ACCOUNTDLLEXPORT TomahawkXmppMessage : public Jreen::Payload
{
    J_PAYLOAD(TomahawkXmppMessage)
    Q_DECLARE_PRIVATE(TomahawkXmppMessage)
    public:
        // sets visible to true
        TomahawkXmppMessage(const QString &ip, unsigned int port, const QString &uniqname, const QString &key);

        // sets visible to false as we dont have any extra information
        TomahawkXmppMessage();
        ~TomahawkXmppMessage();

        const QString ip() const;
        unsigned int port() const;
        const QString uniqname() const;
        const QString key() const;
        bool visible() const;
    private:
        QScopedPointer<TomahawkXmppMessagePrivate> d_ptr;
};

#endif // ENTITYTIME_H
