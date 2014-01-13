/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013-2014, Patrick von Reth <vonreth@kde.org>
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

#ifndef SNORENOTIFYPLUGIN_H
#define SNORENOTIFYPLUGIN_H

#include "../../InfoPluginDllMacro.h"

#include "infosystem/InfoSystem.h"
#include <snore/core/snore.h>

namespace Tomahawk
{

namespace InfoSystem
{

class INFOPLUGINDLLEXPORT SnoreNotifyPlugin : public InfoPlugin
{
    Q_PLUGIN_METADATA( IID "org.tomahawk-player.Player.InfoPlugin" )
    Q_OBJECT
    Q_INTERFACES( Tomahawk::InfoSystem::InfoPlugin )

public:
    SnoreNotifyPlugin();
    virtual ~SnoreNotifyPlugin();


protected slots:
    virtual void init() {}

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

protected slots:
    void slotActionInvoked(Snore::Notification n);

private:
    void notifyUser( InfoType type, const QString &messageText, Snore::Icon icon = Snore::Icon() );
    void addAlert( Tomahawk::InfoSystem::InfoType type, const QString &title );
    Snore::SnoreCore *m_snore;
    Snore::Application m_application;
    Snore::Icon m_defaultIcon;
    QHash< Tomahawk::InfoSystem::InfoType, Snore::Alert > m_alerts;

    void nowPlaying( const QVariant &input );
    void inboxReceived( const QVariant &input );
};

}

}

#endif // SNORENOTIFYPLUGIN_H
