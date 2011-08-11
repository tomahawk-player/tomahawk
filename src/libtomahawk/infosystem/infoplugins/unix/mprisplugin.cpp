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

#include <QApplication>
#include <QtDBus/QtDBus>

#include "audio/audioengine.h"
#include "infosystem/infosystemworker.h"
#include "artist.h"
#include "result.h"
#include "tomahawksettings.h"
#include "globalactionmanager.h"
#include "utils/logger.h"

#include "mprisplugin.h"
#include "mprispluginrootadaptor.h"
#include "mprispluginplayeradaptor.h"

using namespace Tomahawk::InfoSystem;

MprisPlugin::MprisPlugin()
    : InfoPlugin()
{
    qDebug() << Q_FUNC_INFO;

    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;

    new MprisPluginRootAdaptor( this );
    new MprisPluginPlayerAdaptor( this );
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
MprisPlugin::canQuit() const
{
    qDebug() << Q_FUNC_INFO;
    return true;
}

bool
MprisPlugin::canRaise() const
{
    qDebug() << Q_FUNC_INFO;
    return false;
}

bool 
MprisPlugin::hasTrackList() const
{
    qDebug() << Q_FUNC_INFO;
    return false;
}

QString
MprisPlugin::identity() const
{
    return QString("Tomahawk");
}

QString 
MprisPlugin::desktopEntry() const
{
    return QString("tomahawk");
}

QStringList
MprisPlugin::supportedUriSchemes() const
{
    QStringList uriSchemes;
    uriSchemes << "tomahawk" << "spotify";
    return uriSchemes;
}

QStringList
MprisPlugin::supportedMimeTypes() const
{
    return QStringList();
}

void
MprisPlugin::Raise()
{
}

void
MprisPlugin::Quit()
{
    QApplication::quit();
}

// org.mpris.MediaPlayer2.Player

bool
MprisPlugin::canControl() const
{
    return true;
}

bool
MprisPlugin::canGoNext() const
{
    return true;
}

bool
MprisPlugin::canGoPrevious() const
{
    return true;
}

bool
MprisPlugin::canPause() const
{
    return true;
}

bool 
MprisPlugin::canPlay() const
{
    return true;
}

bool
MprisPlugin::canSeek() const
{
    return true;
}

QString
MprisPlugin::loopStatus() const
{
    PlaylistInterface *p = AudioEngine::instance()->playlist();
    if (!p)
        return "None";
    PlaylistInterface::RepeatMode mode = p->repeatMode();
    switch( mode )
    {
        case PlaylistInterface::RepeatOne:
            return "Track";
            break;
        case PlaylistInterface::RepeatAll:
            return "Playlist";
            break;
        case PlaylistInterface::NoRepeat:
            return "None";
            break;
        default:
            return QString("None");
            break;
    }

    return QString("None");
}

void
MprisPlugin::setLoopStatus( const QString &value )
{
    PlaylistInterface *p = AudioEngine::instance()->playlist();
    if (!p)
        return;
    if( value == "Track")
        p->setRepeatMode( PlaylistInterface::RepeatOne );
    else if( value == "Playlist" )
        p->setRepeatMode( PlaylistInterface::RepeatAll );
    else if( value == "None" )
        p->setRepeatMode( PlaylistInterface::NoRepeat );
}

double
MprisPlugin::maximumRate() const
{
    return 1.0;
}

QVariantMap
MprisPlugin::metadata() const
{
    return QVariantMap();
}

double
MprisPlugin::minimumRate() const
{
    return 1.0;
}

QString
MprisPlugin::playbackStatus() const
{
    if( AudioEngine::instance()->state() == AudioEngine::Playing )
        return "Playing";
    else if( AudioEngine::instance()->state() == AudioEngine::Paused )
        return "Paused";
    else if( AudioEngine::instance()->state() == AudioEngine::Stopped )
        return "Stopped";
    return QString("");
}

qlonglong
MprisPlugin::position() const
{
    // Convert Tomahawk's milliseconds to microseconds
    return (qlonglong) ( AudioEngine::instance()->currentTime() * 1000 );
}

double
MprisPlugin::rate() const
{
    return 1.0;
}

void
MprisPlugin::setRate( double value )
{
    Q_UNUSED( value );
}

bool
MprisPlugin::shuffle() const
{
    PlaylistInterface *p = AudioEngine::instance()->playlist();
    if (!p)
        return false;
    return p->shuffled();
}

void
MprisPlugin::setShuffle( bool value )
{
    PlaylistInterface *p = AudioEngine::instance()->playlist();
    if (!p)
        return;
    return p->setShuffled( value );
}

double
MprisPlugin::volume() const
{
    return AudioEngine::instance()->volume();
}

void
MprisPlugin::setVolume( double value )
{
    AudioEngine::instance()->setVolume( value );
}

void
MprisPlugin::Next()
{
    AudioEngine::instance()->next();
}

void 
MprisPlugin::OpenUri(const QString &Uri)
{
    // TODO
}

void
MprisPlugin::Pause()
{
    AudioEngine::instance()->pause();
}

void
MprisPlugin::Play()
{
    AudioEngine::instance()->play();
}

void 
MprisPlugin::PlayPause()
{
    AudioEngine::instance()->playPause();
}

void
MprisPlugin::Previous()
{
    AudioEngine::instance()->previous();
}

void
MprisPlugin::Seek( qlonglong Offset )
{
    qlonglong seekTime = position() + Offset;
    if( seekTime < 0 )
        AudioEngine::instance()->seek( 0 );
    else if( seekTime > AudioEngine::instance()->currentTrackTotalTime() )
        Next();
    // seekTime is in microseconds, but we work internally in milliseconds
    else
        AudioEngine::instance()->seek( (qint64) ( seekTime / 1000 ) );


}

void
MprisPlugin::SetPosition( const QDBusObjectPath &TrackId, qlonglong Position )
{
    // TODO
}

void
MprisPlugin::Stop()
{
    AudioEngine::instance()->stop();
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
