/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,      Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef TOMAHAWK_SCRIPTINFOPLUGIN_H
#define TOMAHAWK_SCRIPTINFOPLUGIN_H

#include "../infosystem/InfoSystem.h"
#include "ScriptPlugin.h"

#include "DllMacro.h"


namespace Tomahawk
{

class JSAccount;
class ScriptInfoPluginPrivate;
class ScriptObject;


class DLLEXPORT ScriptInfoPlugin : public Tomahawk::InfoSystem::InfoPlugin, Tomahawk::ScriptPlugin
{
Q_OBJECT

public:
    /**
     * @param id unique identifier to identify an infoplugin in its scope
     */
    ScriptInfoPlugin( const scriptobject_ptr& scriptObject, const QString& name );
    virtual ~ScriptInfoPlugin();

protected slots:
    void init() override;

    void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData ) override;
    void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData ) override;
    void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData ) override;

    void onGetInfoRequestDone( const QVariantMap& result );
    void onNotInCacheRequestDone( const QVariantMap& result );
    void onCoverArtReturned();

    // boilerplate: to be removed with Qt5 (can be put into ScriptPlugin)
    void onScriptObjectDeleted();

private:
    static QSet< Tomahawk::InfoSystem::InfoType > parseSupportedTypes(const QVariant& variant);
    static QString serializeQVariantMap(const QVariantMap& map);
    static QVariantMap convertInfoStringHashToQVariantMap(const Tomahawk::InfoSystem::InfoStringHash& hash);
    static Tomahawk::InfoSystem::InfoStringHash convertQVariantMapToInfoStringHash( const QVariantMap& map );

    Q_DECLARE_PRIVATE( ScriptInfoPlugin )
    QScopedPointer<ScriptInfoPluginPrivate> d_ptr;
};

}; // ns: Tomahawk

#endif // TOMAHAWK_SCRIPTINFOPLUGIN_H
