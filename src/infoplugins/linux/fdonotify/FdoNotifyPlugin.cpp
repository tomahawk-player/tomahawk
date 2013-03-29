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
#include "utils/TomahawkUtilsGui.h"

#include <QDBusConnection>
#include <QImage>
// QTextDocument provides Qt::escape()
#include <QTextDocument>

namespace Tomahawk
{

namespace InfoSystem
{

FdoNotifyPlugin::FdoNotifyPlugin()
    : InfoPlugin()
    , m_nowPlayingId( 0 )
    , m_wmSupportsBodyMarkup( false )
{
    qDebug() << Q_FUNC_INFO;
    m_supportedPushTypes << InfoNotifyUser << InfoNowPlaying << InfoTrackUnresolved << InfoNowStopped;

    // Query the window manager for its capabilties in styling notifications.
    QDBusMessage message = QDBusMessage::createMethodCall( "org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "GetCapabilities" );
    QDBusConnection::sessionBus().callWithCallback( message, this, SLOT( dbusCapabiltiesReplyReceived( QDBusMessage ) ) );
}


FdoNotifyPlugin::~FdoNotifyPlugin()
{
    qDebug() << Q_FUNC_INFO;
}


void
FdoNotifyPlugin::dbusCapabiltiesReplyReceived( const QDBusMessage& reply )
{
    if ( reply.type() != QDBusMessage::ReplyMessage )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Did not receive a ReplyMessage";
    }

    const QStringList &list = reply.arguments().at( 0 ).toStringList();
    QListIterator<QString> iter( list );
    while ( iter.hasNext() )
    {
        QString capabilty = iter.next();
        if ( capabilty.compare( "body-markup" ) == 0 )
        {
            m_wmSupportsBodyMarkup = true;
            break;
        }
    }
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


/**
 * Determine a consistent size for the cover image depending on the default font height
 */
int
FdoNotifyPlugin::getNotificationIconHeight()
{
     return 6 * TomahawkUtils::defaultFontHeight();
}


void
FdoNotifyPlugin::notifyUser( const QString& messageText )
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
    dict[ "image_data" ] = ImageConverter::variantForImage( QImage( RESPATH "icons/tomahawk-icon-512x512.png" ).scaledToHeight( getNotificationIconHeight() ) );
    arguments << dict; //hints
    arguments << qint32( -1 ); //expire_timeout
    message.setArguments( arguments );
    QDBusConnection::sessionBus().send( message );
}


void
FdoNotifyPlugin::nowPlaying( const QVariant& input )
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

    QString messageText;
    // If the window manager supports notification styling then use it.
    if ( m_wmSupportsBodyMarkup )
    {
        // Remark: If using xml-based markup in notifications, the supplied strings need to be escaped.
        QString album;
        if ( !hash[ "album" ].isEmpty() )
            album = QString( "\n<i>%1</i> %2" ).arg( tr( "on", "'on' is followed by an album name" ) ).arg( Qt::escape( hash[ "album" ] ) );

        messageText = tr( "%1%4 %2%3.", "%1 is a title, %2 is an artist and %3 is replaced by either the previous message or nothing, %4 is the preposition used to link track and artist ('by' in english)" )
                        .arg( Qt::escape( hash[ "title" ] ) )
                        .arg( Qt::escape( hash[ "artist" ] ) )
                        .arg( album )
                        .arg( QString( "\n<i>%1</i>" ).arg( tr( "by", "preposition to link track and artist" ) ) );

        // Dirty hack(TM) so that KNotify/QLabel recognizes the message as Rich Text
        messageText = QString( "<i></i>%1" ).arg( messageText );
    }
    else
    {
        QString album;
        if ( !hash[ "album" ].isEmpty() )
            album = QString( " %1" ).arg( tr( "on \"%1\"", "%1 is an album name" ).arg( hash[ "album" ] ) );

        messageText = tr( "\"%1\" by %2%3.", "%1 is a title, %2 is an artist and %3 is replaced by either the previous message or nothing" )
                        .arg( hash[ "title" ] )
                        .arg( hash[ "artist" ] )
                        .arg( album );
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "sending message" << messageText;

    QDBusMessage message = QDBusMessage::createMethodCall( "org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify" );
    QList<QVariant> arguments;
    arguments << QString( "Tomahawk" ); //app_name
    arguments << m_nowPlayingId; //notification_id
    arguments << QString(); //app_icon
    arguments << QString( "Tomahawk - Now Playing" ); //summary
    arguments << messageText; //body
    arguments << QStringList(); //actions
    QVariantMap dict;
    dict["desktop-entry"] = QString( "tomahawk" );

    // If there is a cover availble use it, else use Tomahawk logo as default.
    QImage image;
    if ( map.contains( "coveruri" ) && map[ "coveruri" ].canConvert< QString >() )
        image = QImage( map[ "coveruri" ].toString(), "PNG" );
    else
        image = QImage( RESPATH "icons/tomahawk-icon-512x512.png" );
    // Convert image to QVariant and scale to a consistent size.
    dict[ "image_data" ] = ImageConverter::variantForImage( image.scaledToHeight( getNotificationIconHeight() ) );

    arguments << dict; //hints
    arguments << qint32( -1 ); //expire_timeout
    message.setArguments( arguments );

    // Handle reply in a callback, so that this a non-blocking call
    QDBusConnection::sessionBus().callWithCallback( message, this, SLOT( dbusPlayingReplyReceived( QDBusMessage ) ) );
}


/**
 * Handle the DBus reply triggered by FdoNotifyPlugin::nowPlaying
 */
void
FdoNotifyPlugin::dbusPlayingReplyReceived( const QDBusMessage& reply )
{
    const QVariantList& list = reply.arguments();
    if ( list.count() > 0 )
        m_nowPlayingId = list.at( 0 ).toInt();
}

} //ns InfoSystem

} //ns Tomahawk

Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::FdoNotifyPlugin )
