/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2016,    Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef TOMAHAWK_SCRIPTLINKPARSERPLUGIN_H
#define TOMAHAWK_SCRIPTLINKPARSERPLUGIN_H

#include "../../resolvers/ScriptPlugin.h"
#include "../../utils/LinkParserPlugin.h"

#include <QObject>

#include "DllMacro.h"

namespace Tomahawk
{

class ScriptObject;
class ScriptLinkParserPluginPrivate;

class DLLEXPORT ScriptLinkParserPlugin : public Utils::LinkParserPlugin, public ScriptPlugin
{
Q_OBJECT

public:
    ScriptLinkParserPlugin( const scriptobject_ptr& scriptObject, ScriptAccount* account );
    virtual ~ScriptLinkParserPlugin();

    bool canParseUrl( const QString& url, Tomahawk::Utils::UrlType type ) const override;
    void lookupUrl( const QString& url ) const override;

private slots:
    void onLookupUrlRequestDone( const QVariantMap& result );
    void pltemplateTracksLoadedForUrl( const QString& url, const playlisttemplate_ptr& pltemplate );

private:
    QString instanceUUID();
    static Tomahawk::query_ptr parseTrack( const QVariantMap& track );

private:
    Q_DECLARE_PRIVATE( ScriptLinkParserPlugin )
    QScopedPointer<ScriptLinkParserPluginPrivate> d_ptr;
};

}; // ns: Tomahawk

#endif // TOMAHAWK_SCRIPTLINKPARSERPLUGIN_H
