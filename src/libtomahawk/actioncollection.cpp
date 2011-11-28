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

#include "actioncollection.h"
#include <tomahawksettings.h>
#include <utils/tomahawkutils.h>
#include "audio/audioengine.h"

ActionCollection* ActionCollection::s_instance = 0;
ActionCollection* ActionCollection::instance()
{
    return s_instance;
}


ActionCollection::ActionCollection( QObject *parent )
    : QObject( parent )
{
    s_instance = this;
    initActions();
}


void
ActionCollection::initActions()
{
    QAction *latchOn = new QAction( tr( "&Listen Along" ), this );
    latchOn->setIcon( QIcon( RESPATH "images/headphones-sidebar.png" ) );
    m_actionCollection[ "latchOn" ] = latchOn;
    QAction *latchOff = new QAction( tr( "Stop &Listening Along" ), this );
    latchOff->setIcon( QIcon( RESPATH "images/headphones-off.png" ) );
    m_actionCollection[ "latchOff" ] = latchOff;

    bool isPublic = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening;
    QAction *privacyToggle = new QAction( ( isPublic ? tr( "&Listen Privately" ) : tr( "&Listen Publicly" ) ), this );
    privacyToggle->setIcon( QIcon( RESPATH "images/private-listening.png" ) );
    privacyToggle->setIconVisibleInMenu( isPublic );
    m_actionCollection[ "togglePrivacy" ] = privacyToggle;
    connect( m_actionCollection[ "togglePrivacy" ], SIGNAL( triggered() ), SLOT( togglePrivateListeningMode() ), Qt::UniqueConnection );


    m_actionCollection[ "loadPlaylist"] = new QAction( tr( "&Load Playlist" ), this );
    m_actionCollection[ "renamePlaylist"] = new QAction( tr( "&Rename Playlist" ), this );
    m_actionCollection[ "copyPlaylist"] = new QAction( tr( "&Copy Playlist Link" ), this );
    m_actionCollection[ "playPause"] = new QAction( tr( "&Play" ), this );
    m_actionCollection[ "stop"] = new QAction( tr( "&Stop" ), this );
    m_actionCollection[ "previousTrack"] = new QAction( tr( "&Previous Track" ), this );
    m_actionCollection[ "nextTrack"] = new QAction( tr( "&Next Track" ), this );
    m_actionCollection[ "quit"] = new QAction( tr( "&Quit" ), this );

    // connect actions to AudioEngine
    AudioEngine *ae = AudioEngine::instance();
    connect( m_actionCollection[ "playPause" ],     SIGNAL( triggered() ), ae,   SLOT( playPause() ),                  Qt::UniqueConnection );
    connect( m_actionCollection[ "stop" ],          SIGNAL( triggered() ), ae,   SLOT( stop() ),                       Qt::UniqueConnection );
    connect( m_actionCollection[ "previousTrack" ], SIGNAL( triggered() ), ae,   SLOT( previous() ),                   Qt::UniqueConnection );
    connect( m_actionCollection[ "nextTrack" ],     SIGNAL( triggered() ), ae,   SLOT( next() ),                       Qt::UniqueConnection );
}


ActionCollection::~ActionCollection()
{
    s_instance = 0;
    foreach( QString key, m_actionCollection.keys() )
        delete m_actionCollection[ key ];
}


QAction*
ActionCollection::getAction( const QString& name )
{
    return m_actionCollection.contains( name ) ? m_actionCollection[ name ] : 0;
}

void
ActionCollection::togglePrivateListeningMode()
{
    tDebug() << Q_FUNC_INFO;
    if ( TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening )
        TomahawkSettings::instance()->setPrivateListeningMode( TomahawkSettings::FullyPrivate );
    else
        TomahawkSettings::instance()->setPrivateListeningMode( TomahawkSettings::PublicListening );

    QAction *privacyToggle = m_actionCollection[ "togglePrivacy" ];
    bool isPublic = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening;
    privacyToggle->setText( ( isPublic ? tr( "&Listen Privately" ) : tr( "&Listen Publicly" ) ) );
    privacyToggle->setIconVisibleInMenu( isPublic );

    emit privacyModeChanged();
}
