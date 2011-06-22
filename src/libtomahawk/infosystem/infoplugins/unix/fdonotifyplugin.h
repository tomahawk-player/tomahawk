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

#ifndef FDONOTIFYPLUGIN_H
#define FDONOTIFYPLUGIN_H

#include "infosystem/infosystem.h"
#include "infosystem/infosystemworker.h"

#include <QtDBus/QDBusArgument>

namespace Tomahawk
{

namespace InfoSystem
{

class FdoNotifyPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    FdoNotifyPlugin();
    virtual ~FdoNotifyPlugin();

    virtual void namChangedSlot( QNetworkAccessManager* ) {}

protected slots:
    virtual void getInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input, const Tomahawk::InfoSystem::InfoCustomData customData )
    {
        Q_UNUSED( caller );
        Q_UNUSED( type );
        Q_UNUSED( input );
        Q_UNUSED( customData );
    }

    virtual void pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant pushData );

    virtual void notInCacheSlot( const Tomahawk::InfoSystem::InfoCriteriaHash criteria, const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input, const Tomahawk::InfoSystem::InfoCustomData customData )
    {
        Q_UNUSED( criteria );
        Q_UNUSED( caller );
        Q_UNUSED( type );
        Q_UNUSED( input );
        Q_UNUSED( customData );
    }

private:
    QDBusArgument m_arg;
};

}

}

#endif // FDONOTIFYPLUGIN_H
