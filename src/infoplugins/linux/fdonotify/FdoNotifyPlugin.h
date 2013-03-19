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

#ifndef FDONOTIFYPLUGIN_H
#define FDONOTIFYPLUGIN_H

#include "infoplugins/InfoPluginDllMacro.h"
#include "infosystem/InfoSystem.h"

#include <QDBusMessage>

namespace Tomahawk
{

namespace InfoSystem
{

class INFOPLUGINDLLEXPORT FdoNotifyPlugin : public InfoPlugin
{
    Q_PLUGIN_METADATA( IID "org.tomahawk-player.Player.InfoPlugin" )
    Q_OBJECT
    Q_INTERFACES( Tomahawk::InfoSystem::InfoPlugin )

public:
    FdoNotifyPlugin();
    virtual ~FdoNotifyPlugin();

protected slots:
    virtual void init() {}

    virtual void dbusPlayingReplyReceived( const QDBusMessage& reply );
    virtual void dbusCapabiltiesReplyReceived( const QDBusMessage& reply );

    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
    {
        Q_UNUSED( requestData );
    }

    virtual void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData );

    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
    {
        Q_UNUSED( criteria );
        Q_UNUSED( requestData );
    }

private:
    int getNotificationIconHeight();

    void notifyUser( const QString& messageText );
    void nowPlaying( const QVariant& input );

    quint32 m_nowPlayingId;

    // Does the window manger support basic XML-based markup (a small HTML subset), see Desktop Notifications specification
    bool m_wmSupportsBodyMarkup;
};

}

}

#endif // FDONOTIFYPLUGIN_H
