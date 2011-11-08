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
    m_actionCollection[ "latchOn" ] = new QAction( tr( "&Listen Along" ), this );
    m_actionCollection[ "latchOff" ] = new QAction( tr( "&Stop Listening Along" ), this );

    bool isPublic = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening;
    QAction *privacyToggle = new QAction( tr( QString( isPublic ? "&Listen Privately" : "&Listen Publicly" ).toAscii().constData() ), this );
    privacyToggle->setIcon( QIcon( RESPATH "images/private-listening.png" ) );
    privacyToggle->setIconVisibleInMenu( isPublic );
    m_actionCollection[ "togglePrivacy" ] = privacyToggle;
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
