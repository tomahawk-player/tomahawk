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

#include "TomahawkSettingsGui.h"

#include <QDesktopServices>
#include "settingsdialog.h"

// #include <QDir>
//
// #include "sip/SipHandler.h"
// #include "playlistinterface.h"
//
// #include "utils/logger.h"
// #include "utils/tomahawkutils.h"
//
// #include "database/databasecommand_updatesearchindex.h"
// #include "database/database.h"

#define VERSION 5

using namespace Tomahawk;

inline QDataStream& operator<<(QDataStream& out, const AtticaManager::StateHash& states)
{
    out <<  VERSION;
    out << (quint32)states.count();
    foreach( const QString& key, states.keys() )
    {
        AtticaManager::Resolver resolver = states[ key ];
        out << key << resolver.version << resolver.scriptPath << (qint32)resolver.state << resolver.userRating;
    }
    return out;
}


inline QDataStream& operator>>(QDataStream& in, AtticaManager::StateHash& states)
{
    quint32 count = 0, version = 0;
    in >> version;
    in >> count;
    for ( uint i = 0; i < count; i++ )
    {
        QString key, version, scriptPath;
        qint32 state, userRating;
        in >> key;
        in >> version;
        in >> scriptPath;
        in >> state;
        in >> userRating;
        states[ key ] = AtticaManager::Resolver( version, scriptPath, userRating, (AtticaManager::ResolverState)state );
    }
    return in;
}

TomahawkSettingsGui*
TomahawkSettingsGui::instanceGui()
{
    return qobject_cast< TomahawkSettingsGui* >(TomahawkSettings::instance());
}


TomahawkSettingsGui::TomahawkSettingsGui( QObject* parent )
    : TomahawkSettings( parent )
{
    qRegisterMetaType< AtticaManager::StateHash >( "AtticaManager::StateHash" );
    qRegisterMetaTypeStreamOperators<AtticaManager::StateHash>("AtticaManager::StateHash");
}


TomahawkSettingsGui::~TomahawkSettingsGui()
{
}


QString
TomahawkSettingsGui::storageCacheLocation() const
{
    return QDesktopServices::storageLocation( QDesktopServices::CacheLocation ) + "/InfoSystemCache/";
}


QStringList
TomahawkSettingsGui::scannerPaths() const
{
    QString musicLocation;

    musicLocation = QDesktopServices::storageLocation( QDesktopServices::MusicLocation );

    return value( "scanner/paths", musicLocation ).toStringList();
}


void
TomahawkSettingsGui::setAtticaResolverState( const QString& resolver, AtticaManager::ResolverState state )
{
    AtticaManager::StateHash resolvers = value( "script/atticaresolverstates" ).value< AtticaManager::StateHash >();
    AtticaManager::Resolver r = resolvers.value( resolver );
    r.state = state;
    resolvers.insert( resolver, r );
    setValue( "script/atticaresolverstates", QVariant::fromValue< AtticaManager::StateHash >( resolvers ) );

    sync();
}

AtticaManager::StateHash
TomahawkSettingsGui::atticaResolverStates() const
{
    return value( "script/atticaresolverstates" ).value< AtticaManager::StateHash >();
}

void
TomahawkSettingsGui::setAtticaResolverStates( const AtticaManager::StateHash states )
{
    setValue( "script/atticaresolverstates", QVariant::fromValue< AtticaManager::StateHash >( states ) );
}


void
TomahawkSettingsGui::removeAtticaResolverState ( const QString& resolver )
{
    AtticaManager::StateHash resolvers = value( "script/atticaresolverstates" ).value< AtticaManager::StateHash >();
    resolvers.remove( resolver );
    setValue( "script/atticaresolverstates", QVariant::fromValue< AtticaManager::StateHash >( resolvers ) );
}
