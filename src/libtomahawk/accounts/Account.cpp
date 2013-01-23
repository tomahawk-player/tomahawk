/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "Account.h"

#include "TomahawkSettings.h"

namespace Tomahawk
{

namespace Accounts
{

QString
accountTypeToString( AccountType type )
{
    switch ( type )
    {
        case SipType:
            return QObject::tr( "Friend Finders" );
        case ResolverType:
            return QObject::tr( "Music Finders" );
        case InfoType:
        case StatusPushType:
            return QObject::tr( "Status Updaters" );
        case NoType:
            return QString();
    }

    return QString();
}


Account::Account( const QString& accountId )
    : QObject()
    , m_enabled( false )
    , m_accountId( accountId )
{
    connect( this, SIGNAL( error( int, QString ) ), this, SLOT( onError( int,QString ) ) );
    connect( this, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) , this, SLOT( onConnectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) );

    loadFromConfig( accountId );
}


Account::~Account()
{
}


#ifndef ENABLE_HEADLESS

AccountConfigWidget*
Account::configurationWidget()
{
    return 0;
}


QWidget*
Account::aclWidget()
{
    return 0;
}


QPixmap
Account::icon() const
{
    return QPixmap();
}

#endif


void
Account::authenticate()
{
    return;
}


void
Account::deauthenticate()
{
    return;
}


bool
Account::isAuthenticated() const
{
    return false;
}


void
Account::onError( int errorCode, const QString& error )
{
    Q_UNUSED( errorCode );

    QMutexLocker locker( &m_mutex );
    m_cachedError = error;
}


void
Account::onConnectionStateChanged( Account::ConnectionState )
{
    m_cachedError.clear();
}


void
Account::syncConfig()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "accounts/" + m_accountId );
    s->setValue( "accountfriendlyname", m_accountFriendlyName );
    s->setValue( "enabled", m_enabled );
    s->setValue( "credentials", m_credentials );
    s->setValue( "configuration", m_configuration );
    s->setValue( "acl", m_acl );
    s->setValue( "types", m_types );
    s->endGroup();
    s->sync();
}


void
Account::loadFromConfig( const QString& accountId )
{
    m_accountId = accountId;
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "accounts/" + m_accountId );
    m_accountFriendlyName = s->value( "accountfriendlyname", QString() ).toString();
    m_enabled = s->value( "enabled", false ).toBool();
    m_credentials = s->value( "credentials", QVariantHash() ).toHash();
    m_configuration = s->value( "configuration", QVariantHash() ).toHash();
    m_acl = s->value( "acl", QVariantMap() ).toMap();
    m_types = s->value( "types", QStringList() ).toStringList();
    s->endGroup();
}


void
Account::removeFromConfig()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->beginGroup( "accounts/" + m_accountId );
    s->remove( "accountfriendlyname" );
    s->remove( "enabled" );
    s->remove( "credentials" );
    s->remove( "configuration" );
    s->remove( "acl" );
    s->remove( "types" );
    s->endGroup();
    s->remove( "accounts/" + m_accountId );
}


void
Account::setTypes( AccountTypes types )
{
    QMutexLocker locker( &m_mutex );
    m_types = QStringList();
    if ( types & InfoType )
        m_types << "InfoType";
    if ( types & SipType )
        m_types << "SipType";
    if ( types & ResolverType )
        m_types << "ResolverType";
    if ( types & StatusPushType )
        m_types << "StatusPushType";
    syncConfig();
}


AccountTypes
Account::types() const
{
    QMutexLocker locker( &m_mutex );
    AccountTypes types;
    if ( m_types.contains( "InfoType" ) )
        types |= InfoType;
    if ( m_types.contains( "SipType" ) )
        types |= SipType;
    if ( m_types.contains( "ResolverType" ) )
        types |= ResolverType;
    if ( m_types.contains( "StatusPushType" ) )
        types |= StatusPushType;

    return types;
}


}

}
