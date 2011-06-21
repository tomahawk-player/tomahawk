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

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

using namespace Tomahawk::InfoSystem;

LibNotifyPlugin::LibNotifyPlugin()
    : InfoPlugin()
{
    qDebug() << Q_FUNC_INFO;
    m_supportedPushTypes << Tomahawk::InfoSystem::InfoNotifyUser;
}

LibNotifyPlugin::~LibNotifyPlugin()
{
    qDebug() << Q_FUNC_INFO;
}

void
LibNotifyPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant data )
{
    qDebug() << Q_FUNC_INFO;
    if ( type != Tomahawk::InfoSystem::InfoNotifyUser || !data.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        qDebug() << Q_FUNC_INFO << " not the right type or could not convert the hash";
        return;
    }
    Tomahawk::InfoSystem::InfoCriteriaHash hash = data.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "message" ) )
    {
        qDebug() << Q_FUNC_INFO << " hash did not contain a message";
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall( "org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify" );
    QList<QVariant> arguments;
    arguments << QString( "Tomahawk" ); //app_name
    arguments << quint32( 0 ); //notification_id
    arguments << QString(); //app_icon
    arguments << caller; //summary
    arguments << hash["message"]; //body
    arguments << QStringList(); //actions
    QMap< QString, QVariant > dict;
    dict["desktop-entry"] = QString( "tomahawk" );
    arguments << dict; //hints
    arguments << quint32( -1 ); //expire_timeout
    message.setArguments( arguments );
    QDBusConnection::sessionBus().send( message );
}
