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

#include <string.h>

#include "infosystem/infosystemworker.h"
#include "artist.h"
#include "result.h"
#include "tomahawksettings.h"

#include "adiumplugin.h"
#include "adium.h"

QString adium_beforeStatus;
QString adium_afterStatus;

static void setStatus(const QString &status)
{
    // The command that updates the status
    QString scriptqstr;
    scriptqstr.append(adium_beforeStatus);
    scriptqstr.append(status);
    scriptqstr.append(adium_afterStatus);

    const char* scriptstr = scriptqstr.toUtf8();
    script( scriptstr );
}

using namespace Tomahawk::InfoSystem;

AdiumPlugin::AdiumPlugin()
    : InfoPlugin()
{
    qDebug() << Q_FUNC_INFO;

    adium_beforeStatus = "if appIsRunning(\"Adium\") then\n";
    adium_beforeStatus.append("tell application \"Adium\"\n");
    adium_beforeStatus.append("set the status message of every account to \"");

    adium_afterStatus.append("\"\nend tell\n");
    adium_afterStatus.append("end if\n");
    adium_afterStatus.append("on appIsRunning(appName)\n");
    adium_afterStatus.append("tell application \"System Events\" to (name of processes) contains appName\n");
    adium_afterStatus.append("end appIsRunning\n");

    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;

    m_active = TomahawkSettings::instance()->nowPlayingEnabled();

    connect( TomahawkSettings::instance(), SIGNAL( changed() ),
                                             SLOT( settingsChanged() ), Qt::QueuedConnection );
}

AdiumPlugin::~AdiumPlugin()
{
    qDebug() << Q_FUNC_INFO;
    setStatus( "" );
}

void
AdiumPlugin::settingsChanged()
{
    m_active = TomahawkSettings::instance()->nowPlayingEnabled();
    if( !m_active )
        setStatus( "" );
}

void
AdiumPlugin::getInfo( const QString caller, const InfoType type, const QVariant data, InfoCustomData customData )
{
    switch (type)
    {
        default:
        {
            emit info(caller, Tomahawk::InfoSystem::InfoNoInfo, QVariant(), QVariant(), customData);
            return;
        }
    }
}

void
AdiumPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input )
{
    qDebug() << Q_FUNC_INFO;

    if( !m_active )
        return;

    switch ( type )
    {
        case InfoNowPlaying:
          audioStarted( input );
          break;
        case InfoNowPaused:
          audioPaused();
          break;
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
AdiumPlugin::audioStarted( const QVariant &input )
{
    qDebug() << Q_FUNC_INFO;

    if ( !input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
        return;

    InfoCriteriaHash hash = input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) )
        return;

    QString nowPlaying = "";
    nowPlaying.append( hash["title"] );
    nowPlaying.append(" - ");
    nowPlaying.append( hash["artist"] );
    nowPlaying.append( " " );
    nowPlaying.append( openLinkFromHash( hash ).toString() );
    setStatus( nowPlaying );
}

void
AdiumPlugin::audioFinished( const QVariant &input )
{
    //qDebug() << Q_FUNC_INFO;
}

void 
AdiumPlugin::audioStopped()
{
    qDebug() << Q_FUNC_INFO;
    // TODO: audio stopped, so push update status to Adium that says "stopped"
    setStatus( "Stopped" );
}

void 
AdiumPlugin::audioPaused()
{
    qDebug() << Q_FUNC_INFO;
    // TODO: audio paused, so push update status to Adium that says "paused"
    setStatus( "Paused" );
}

void 
AdiumPlugin::audioResumed( const QVariant &input )
{
    qDebug() << Q_FUNC_INFO;
    // TODO: audio resumed, so push update status to Adium with playing track
    audioStarted( input );
}
