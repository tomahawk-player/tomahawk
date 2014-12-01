/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,    Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef TOMAHAWK_SCRIPTLINKGENERATORPLUGIN_H
#define TOMAHAWK_SCRIPTLINKGENERATORPLUGIN_H

#include "../utils/LinkGeneratorPlugin.h"

#include "DllMacro.h"


namespace Tomahawk
{

class ScriptObject;
class ScriptLinkGeneratorPluginPrivate;

class DLLEXPORT ScriptLinkGeneratorPlugin : QObject, public Utils::LinkGeneratorPlugin
{
Q_OBJECT

public:
    ScriptLinkGeneratorPlugin( ScriptObject* scriptObject );
    virtual ~ScriptLinkGeneratorPlugin();

    ScriptJob* openLink( const QString& title, const QString& artist, const QString& album ) const override;
    ScriptJob* openLink( const artist_ptr& artist ) const override;
    ScriptJob* openLink( const album_ptr& album ) const override;
    ScriptJob* openLink( const dynplaylist_ptr& playlist ) const override;

private:
    Q_DECLARE_PRIVATE( ScriptLinkGeneratorPlugin )
    QScopedPointer<ScriptLinkGeneratorPluginPrivate> d_ptr;
};

}; // ns: Tomahawk

#endif // TOMAHAWK_SCRIPTLINKGENERATORPLUGIN_H
