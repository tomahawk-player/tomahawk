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

#include "libnotifyplugin.h"

#include "utils/tomahawkutils.h"

#include <libnotify/notify.h>
#include <glib-2.0/glib.h>

using namespace Tomahawk::InfoSystem;

// for internal neatness

LibNotifyPlugin::LibNotifyPlugin()
    : InfoPlugin()
    , m_isInited( false )
{
    qDebug() << Q_FUNC_INFO;
    m_supportedGetTypes << Tomahawk::InfoSystem::InfoNotifyUser;
    gboolean initSuccess = notify_init( "Tomahawk" );
    m_isInited = ( initSuccess == TRUE );
}

LibNotifyPlugin::~LibNotifyPlugin()
{
    qDebug() << Q_FUNC_INFO;
}

void
LibNotifyPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant data )
{
    qDebug() << Q_FUNC_INFO;
    if ( type != Tomahawk::InfoSystem::InfoNotifyUser || !data.canConvert< Tomahawk::InfoSystem::InfoCustomData >() )
        return;
    Tomahawk::InfoSystem::InfoCustomData hash = data.value< Tomahawk::InfoSystem::InfoCustomData >();
    if ( !hash.contains( "message" ) || !(hash["message"].canConvert< QString >() ) )
        return;
    QString message = hash["trackName"].toString();
}
