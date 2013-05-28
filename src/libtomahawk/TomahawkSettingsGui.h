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

#ifndef TOMAHAWK_SETTINGS_GUI_H
#define TOMAHAWK_SETTINGS_GUI_H

#include "TomahawkSettings.h"
#include "AtticaManager.h"
//
// #include <QSettings>

#include "DllMacro.h"

/**
 * Convenience wrapper around QSettings for tomahawk-specific config
 */
class DLLEXPORT TomahawkSettingsGui : public TomahawkSettings
{
Q_OBJECT

public:
    static TomahawkSettingsGui* instanceGui();

    explicit TomahawkSettingsGui( QObject* parent = 0 );
    virtual ~TomahawkSettingsGui();

    virtual QString storageCacheLocation() const;
    virtual QStringList scannerPaths() const;

    AtticaManager::StateHash atticaResolverStates() const;
    void setAtticaResolverStates( const AtticaManager::StateHash states );

    void setAtticaResolverState( const QString& resolver, AtticaManager::ResolverState state );
    void removeAtticaResolverState( const QString& resolver );

    static void registerCustomSettingsHandlers();
};

Q_DECLARE_METATYPE(AtticaManager::StateHash);


#endif
