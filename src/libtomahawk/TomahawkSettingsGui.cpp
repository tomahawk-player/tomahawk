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

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

using namespace Tomahawk;

inline QDataStream& operator<<(QDataStream& out, const AtticaManager::StateHash& states)
{
    out <<  TOMAHAWK_SETTINGS_VERSION;
    out << (quint32)states.count();
    foreach( const QString& key, states.keys() )
    {
        AtticaManager::Resolver resolver = states[ key ];
        out << key << resolver.version << resolver.scriptPath << (qint32)resolver.state << resolver.userRating << resolver.binary;
    }
    return out;
}


inline QDataStream& operator>>(QDataStream& in, AtticaManager::StateHash& states)
{
    quint32 count = 0, configVersion = 0;
    in >> configVersion;
    in >> count;
    for ( uint i = 0; i < count; i++ )
    {
        QString key, version, scriptPath;
        qint32 state, userRating;
        bool binary = false;
        in >> key;
        in >> version;
        in >> scriptPath;
        in >> state;
        in >> userRating;
        if ( configVersion > 10 )
        {
            // V11 includes 'bool binary' flag
            in >> binary;
        }
        states[ key ] = AtticaManager::Resolver( version, scriptPath, userRating, (AtticaManager::ResolverState)state, binary );
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
}


TomahawkSettingsGui::~TomahawkSettingsGui()
{
}


QString
TomahawkSettingsGui::storageCacheLocation() const
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    return QStandardPaths::writableLocation( QStandardPaths::CacheLocation );
#else
    return QDesktopServices::storageLocation( QDesktopServices::CacheLocation );
#endif
}


QStringList
TomahawkSettingsGui::scannerPaths() const
{
    QString musicLocation;

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    musicLocation = QStandardPaths::writableLocation( QStandardPaths::MusicLocation );
#else
    musicLocation = QDesktopServices::storageLocation( QDesktopServices::MusicLocation );
#endif

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


void
TomahawkSettingsGui::registerCustomSettingsHandlers()
{
    qRegisterMetaType< AtticaManager::StateHash >( "AtticaManager::StateHash" );
    qRegisterMetaTypeStreamOperators<AtticaManager::StateHash>("AtticaManager::StateHash");
}
