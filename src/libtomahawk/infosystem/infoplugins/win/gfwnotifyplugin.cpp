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

#include "gfwnotifyplugin.h"
#include "utils/tomahawkutils.h"
#include "headlesscheck.h"

#include "growl.h"
#include "growl++.hpp"

using namespace Tomahawk::InfoSystem;

GfwNotifyPlugin::GfwNotifyPlugin()
    : InfoPlugin()
    , m_growl( 0 )
{
    const char* notifications[1];
    const char* name = "Notification";
    notifications[0] = name;
    const char* host = "127.0.0.1";
    const char* password = "";
    const char* application = "Tomahawk";
    m_growl = new Growl( GROWL_TCP, host, password, application, notifications, 1 );
    qDebug() << Q_FUNC_INFO;
    m_supportedPushTypes << Tomahawk::InfoSystem::InfoNotifyUser;
}

GfwNotifyPlugin::~GfwNotifyPlugin()
{
    qDebug() << Q_FUNC_INFO;
}

void
GfwNotifyPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant pushData )
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

    const char* name = "Notification";
    const char* title = "Tomahawk";
    const char* message = strdup( hash["message"].toLocal8Bit().constData() );
    const char* url = "";
    const char* icon = QString( "file:///" + TOMAHAWK_APPLICATION::applicationDirPath() + "/tomahawk-icon-128x128.png" ).toLocal8Bit().constData();
    m_growl->Notify( name, title, message, url, icon );
}
