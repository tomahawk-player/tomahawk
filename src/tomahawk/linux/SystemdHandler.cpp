/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "SystemdHandler.h"

#include "accounts/AccountManager.h"
#include "utils/Logger.h"

#include <QTimer>
#include <QtSystemd/ldmanager.h>

#define SYSTEMD_RESUME_DELAY 2000

namespace Tomahawk {

SystemdHandler::SystemdHandler(QObject *parent) :
    QObject(parent)
{
    connect( Systemd::Logind::notifier(), SIGNAL( prepareForSleep( bool ) ),
             SLOT( handleSleep( bool ) ) );
}


void
SystemdHandler::handleSleep( bool active )
{
    if ( active )
    {
        QMutexLocker locker( &m_mutex );
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "About to sleep so disconnecting all accounts";
        Tomahawk::Accounts::AccountManager::instance()->disconnectAll();
    }
    else
    {
        m_mutex.lock();
        // Delay resuming for other wakeup actions, e.g. reconnecting to the network, to take place.
        QTimer::singleShot( SYSTEMD_RESUME_DELAY, this, SLOT( actualResume() ) );
    }

}


void
SystemdHandler::actualResume()
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Awake from sleep so connecting all accounts";
    Tomahawk::Accounts::AccountManager::instance()->connectAll();
    m_mutex.unlock();
}

} // namespace Tomahawk
