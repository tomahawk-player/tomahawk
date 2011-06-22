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

// Marked portions of this file are subject to the following copyright:
/*
 * Copyright (C) 2009 by Aurélien Gâteau <aurelien.gateau@canonical.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "fdonotifyplugin.h"
#include "utils/tomahawkutils.h"
#include "imageconverter.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QImage>

using namespace Tomahawk::InfoSystem;

FdoNotifyPlugin::FdoNotifyPlugin()
    : InfoPlugin()
    , m_arg()
{
    qDebug() << Q_FUNC_INFO;
    m_supportedPushTypes << Tomahawk::InfoSystem::InfoNotifyUser;
}

FdoNotifyPlugin::~FdoNotifyPlugin()
{
    qDebug() << Q_FUNC_INFO;
}

void
FdoNotifyPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant pushData )
{
    Q_UNUSED( caller );
    qDebug() << Q_FUNC_INFO;
    if ( type != Tomahawk::InfoSystem::InfoNotifyUser || !pushData.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        qDebug() << Q_FUNC_INFO << " not the right type or could not convert the hash";
        return;
    }
    Tomahawk::InfoSystem::InfoCriteriaHash hash = pushData.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
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
    arguments << QString( "Tomahawk" ); //summary
    arguments << hash["message"]; //body
    arguments << QStringList(); //actions
    QVariantMap dict;
    dict["desktop-entry"] = QString( "tomahawk" );
    dict["image_data"] = ImageConverter::variantForImage( QImage( RESPATH "icons/tomahawk-icon-128x128.png" ) );
    arguments << dict; //hints
    arguments << qint32( -1 ); //expire_timeout
    message.setArguments( arguments );
    QDBusConnection::sessionBus().send( message );
}
