/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *             2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *             2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "sip/SipPlugin.h"

#include "utils/Logger.h"
#include "Source.h"


SipPlugin::SipPlugin() : QObject() {}
SipPlugin::~SipPlugin() {}

SipPlugin::SipPlugin( Tomahawk::Accounts::Account *account, QObject* parent )
    : QObject( parent )
    , m_account( account )
{
    connect( this, SIGNAL( peerOnline( QString ) ), this, SLOT( onPeerOnline( QString ) ) );
    connect( this, SIGNAL( peerOffline( QString ) ), this, SLOT( onPeerOffline( QString ) ) );
}


QString
SipPlugin::pluginId() const
{
    return m_account->accountId();
}


const QString
SipPlugin::friendlyName() const
{
    return m_account->accountFriendlyName();
}


const QString
SipPlugin::serviceName() const
{
    return m_account->accountServiceName();
}

QString
SipPlugin::inviteString() const
{
    return QString();
}


#ifndef ENABLE_HEADLESS

QMenu*
SipPlugin::menu()
{
    return 0;
}

#endif


Tomahawk::Accounts::Account*
SipPlugin::account() const
{
    return m_account;
}


const QStringList
SipPlugin::peersOnline() const
{
    return m_peersOnline;
}


void
SipPlugin::onPeerOnline( const QString& peerId )
{
   if( !m_peersOnline.contains( peerId ) )
   {
       m_peersOnline.append( peerId );
   }
}


void
SipPlugin::onPeerOffline( const QString& peerId )
{
    m_peersOnline.removeAll( peerId );
}
