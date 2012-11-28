/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "FdoNotifyPlugin.h"
#include "utils/TomahawkUtils.h"
#include "ImageConverter.h"

#include "TomahawkSettings.h"

#include "utils/Logger.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtGui/QImage>
#include <QtPlugin>

namespace Tomahawk
{

namespace InfoSystem
{

FdoNotifyPlugin::FdoNotifyPlugin()
    : InfoPlugin()
    , m_nowPlayingId( 0 )
{
    qDebug() << Q_FUNC_INFO;
    m_supportedPushTypes << InfoNotifyUser << InfoNowPlaying << InfoTrackUnresolved << InfoNowStopped;
}


FdoNotifyPlugin::~FdoNotifyPlugin()
{
    qDebug() << Q_FUNC_INFO;
}


void
FdoNotifyPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    qDebug() << Q_FUNC_INFO << "showing notification: " << TomahawkSettings::instance()->songChangeNotificationEnabled();

    if ( !TomahawkSettings::instance()->songChangeNotificationEnabled() )
        return;

    QVariant inputData = pushData.infoPair.second;

    switch ( pushData.type )
    {
        case Tomahawk::InfoSystem::InfoTrackUnresolved:
            notifyUser( "The current track could not be resolved. Tomahawk will pick back up with the next resolvable track from this source." );
            return;

        case Tomahawk::InfoSystem::InfoNotifyUser:
            notifyUser( pushData.infoPair.second.toString() );
            return;

        case Tomahawk::InfoSystem::InfoNowStopped:
            notifyUser( "Tomahawk is stopped." );
            return;

        case Tomahawk::InfoSystem::InfoNowPlaying:
            nowPlaying( pushData.infoPair.second );
            return;

        default:
            return;
    }

}


void
FdoNotifyPlugin::notifyUser( const QString &messageText )
{
    QDBusMessage message = QDBusMessage::createMethodCall( "org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify" );
    QList<QVariant> arguments;
    arguments << QString( "Tomahawk" ); //app_name
    arguments << quint32( 0 ); //notification_id
    arguments << QString(); //app_icon
    arguments << QString( "Tomahawk" ); //summary
    arguments << QString( messageText ); //body
    arguments << QStringList(); //actions
    QVariantMap dict;
    dict["desktop-entry"] = QString( "tomahawk" );
    dict[ "image_data" ] = ImageConverter::variantForImage( QImage( RESPATH "icons/tomahawk-icon-128x128.png" ) );
    arguments << dict; //hints
    arguments << qint32( -1 ); //expire_timeout
    message.setArguments( arguments );
    QDBusConnection::sessionBus().send( message );
}


void
FdoNotifyPlugin::nowPlaying( const QVariant &input )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( !input.canConvert< QVariantMap >() )
        return;

    QVariantMap map = input.toMap();

    if ( !map.contains( "trackinfo" ) || !map[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
        return;

    InfoStringHash hash = map[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) )
        return;

    QString messageText = tr( "Tomahawk is playing \"%1\" by %2%3." )
                        .arg( hash[ "title" ] )
                        .arg( hash[ "artist" ] )
                        .arg( hash[ "album" ].isEmpty() ? QString() : QString( " %1" ).arg( tr( "on \"%1\"" ).arg( hash[ "album" ] ) ) );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "sending message" << messageText;

    QDBusMessage message = QDBusMessage::createMethodCall( "org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify" );
    QList<QVariant> arguments;
    arguments << QString( "Tomahawk" ); //app_name
    arguments << m_nowPlayingId; //notification_id
    arguments << QString(); //app_icon
    arguments << QString( "Tomahawk" ); //summary
    arguments << messageText; //body
    arguments << QStringList(); //actions
    QVariantMap dict;
    dict["desktop-entry"] = QString( "tomahawk" );
    if ( map.contains( "coveruri" ) && map[ "coveruri" ].canConvert< QString >() )
        dict[ "image_data" ] = ImageConverter::variantForImage( QImage( map[ "coveruri" ].toString(), "PNG" ) );
    else
        dict[ "image_data" ] = ImageConverter::variantForImage( QImage( RESPATH "icons/tomahawk-icon-128x128.png" ) );
    arguments << dict; //hints
    arguments << qint32( -1 ); //expire_timeout
    message.setArguments( arguments );

    const QDBusMessage &reply = QDBusConnection::sessionBus().call( message );
    const QVariantList &list = reply.arguments();
    if ( list.count() > 0 )
        m_nowPlayingId = list.at( 0 ).toInt();
}

} //ns InfoSystem

} //ns Tomahawk

Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::FdoNotifyPlugin )
