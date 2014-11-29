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

#ifndef TOMAHAWK_JSINFOPLUGIN_H
#define TOMAHAWK_JSINFOPLUGIN_H

#include "../infosystem/InfoSystem.h"

#include "DllMacro.h"


namespace Tomahawk
{

class JSPlugin;
class JSInfoPluginPrivate;


class DLLEXPORT JSInfoPlugin : public Tomahawk::InfoSystem::InfoPlugin
{
Q_OBJECT

public:
    /**
     * @param id unique identifier to identify an infoplugin in its scope
     */
    JSInfoPlugin( int id, JSPlugin* resolver );
    virtual ~JSInfoPlugin();


    Q_INVOKABLE void addInfoRequestResult( int requestId, qint64 maxAge, const QVariantMap& returnedData );
    Q_INVOKABLE void emitGetCachedInfo( int requestId, const QVariantMap& criteria, int newMaxAge );
    Q_INVOKABLE void emitInfo( int requestId, const QVariantMap& output );

protected slots:
    void init() override;

    void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData ) override;
    void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData ) override;
    void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData ) override;

protected:
    // TODO: create JSPlugin base class and move these methods there
    QString serviceGetter() const; // = 0
    void callMethodOnInfoPlugin( const QString& scriptSource );
    QVariant callMethodOnInfoPluginWithResult( const QString& scriptSource );

private:
    static QSet< Tomahawk::InfoSystem::InfoType > parseSupportedTypes(const QVariant& variant);
    static QString serializeQVariantMap(const QVariantMap& map);
    static QVariantMap convertInfoStringHashToQVariantMap(const Tomahawk::InfoSystem::InfoStringHash& hash);
    static Tomahawk::InfoSystem::InfoStringHash convertQVariantMapToInfoStringHash( const QVariantMap& map );

    Q_DECLARE_PRIVATE( JSInfoPlugin )
    QScopedPointer<JSInfoPluginPrivate> d_ptr;
};

}; // ns: Tomahawk

#endif // TOMAHAWK_JSINFOPLUGIN_H
