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

#include "accounts/AccountManager.h"
#include "utils/Logger.h"
#include "UPowerHandler.h"

#include <QTimer>

#define UPOWER_RESUME_DELAY 2000

using namespace Tomahawk;

const char* UPowerHandler::UPowerService = "org.freedesktop.UPower";
const char* UPowerHandler::UPowerPath = "/org/freedesktop/UPower";
const char* UPowerHandler::UPowerInterface = "org.freedesktop.UPower";

Tomahawk::UPowerHandler::UPowerHandler( QObject *parent )
    : QObject( parent )
{
}

bool
UPowerHandler::registerHandler()
{
    // Check if the UPower is available
    if ( !QDBusConnection::systemBus().interface()->isServiceRegistered( UPowerService ) ) {
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "UPower is not available";
        return false;
    }

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "UPower available, will reconnect on wake from suspend.";
    if ( m_interface.isNull() )
    {
        m_interface = QSharedPointer<org::freedesktop::UPower>( new org::freedesktop::UPower( UPowerService, UPowerPath, QDBusConnection::systemBus(), this ) );
    }
    connect( m_interface.data(), SIGNAL( Sleeping() ), this, SLOT( handleSleep() ));
    connect( m_interface.data(), SIGNAL( Resuming() ), this, SLOT( handleResume() ));
    return true;
}

void
UPowerHandler::handleSleep()
{
    QMutexLocker locker( &m_mutex );
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "About to sleep so disconnecting all accounts";
    Tomahawk::Accounts::AccountManager::instance()->disconnectAll();
}

void
UPowerHandler::handleResume()
{
    m_mutex.lock();
    // Delay resuming for other wakeup actions, e.g. reconnecting to the network, to take place.
    QTimer::singleShot( UPOWER_RESUME_DELAY, this, SLOT( actualResume() ) );
}

void
UPowerHandler::actualResume()
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Awake from sleep so connecting all accounts";
    Tomahawk::Accounts::AccountManager::instance()->connectAll();
    m_mutex.unlock();
}
