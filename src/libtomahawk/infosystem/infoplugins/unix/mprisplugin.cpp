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

#include <QtDBus/QtDBus>

#include "infosystem/infosystemworker.h"
#include "artist.h"
#include "result.h"
#include "tomahawksettings.h"
#include "globalactionmanager.h"
#include "utils/logger.h"

#include "mprisplugin.h"
#include "mprispluginrootadaptor.h"

using namespace Tomahawk::InfoSystem;

MprisPlugin::MprisPlugin()
    : InfoPlugin()
{
    qDebug() << Q_FUNC_INFO;

    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;

    new MprisPluginRootAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/org/mpris/MediaPlayer2", this);
    dbus.registerService("org.mpris.MediaPlayer2.tomahawk");

}


MprisPlugin::~MprisPlugin()
{
    qDebug() << Q_FUNC_INFO;
}

// org.mpris.MediaPlayer2

bool
MprisPlugin::canQuit()
{
    qDebug() << Q_FUNC_INFO;
    return true;
}

bool
MprisPlugin::canRaise()
{
    return false;
}

bool 
MprisPlugin::hasTrackList()
{
    return false;
}
QString
MprisPlugin::identity()
{
    return QString("Tomahawk Music Player");
}

QString 
MprisPlugin::desktopEntry()
{
    return QString("tomahawk");
}

QStringList
MprisPlugin::supportedUriSchemes()
{
    return QStringList();
}

QStringList
MprisPlugin::supportedMimeTypes()
{
    return QStringList();
}

void
MprisPlugin::raise()
{
}

void
MprisPlugin::quit()
{
}

// InfoPlugin Methods

void
MprisPlugin::getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
  qDebug() << Q_FUNC_INFO;

  return;
}

void
MprisPlugin::pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input )
{
    qDebug() << Q_FUNC_INFO;

    switch ( type )
    {
        case InfoNowPlaying:
          audioStarted( input );
          break;
        case InfoNowPaused:
          audioPaused();
          return;
        case InfoNowResumed:
          audioResumed( input );
          break;
        case InfoNowStopped:
          audioStopped();
          break;

        default:
          return;
    }

}

/** Audio state slots */
void
MprisPlugin::audioStarted( const QVariant &input )
{
    qDebug() << Q_FUNC_INFO;

    if ( !input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
        return;

    InfoCriteriaHash hash = input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) )
        return;

    //hash["artist"];
    //hash["title"];
    QString nowPlaying = "";
    qDebug() << "nowPlaying: " << nowPlaying;
}

void
MprisPlugin::audioFinished( const QVariant &input )
{
    //qDebug() << Q_FUNC_INFO;
}

void
MprisPlugin::audioStopped()
{
    qDebug() << Q_FUNC_INFO;
}

void
MprisPlugin::audioPaused()
{
    qDebug() << Q_FUNC_INFO;
}

void
MprisPlugin::audioResumed( const QVariant &input )
{
    qDebug() << Q_FUNC_INFO;
    audioStarted( input );
}
