/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
*
*   Copyright 2014, Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef TOMAHAWK_JSINFOSYSTEMHELPER_H
#define TOMAHAWK_JSINFOSYSTEMHELPER_H

#include <QObject>

namespace Tomahawk
{

class JSPlugin;
class JSInfoSystemHelperPrivate;

class JSInfoSystemHelper : public QObject
{
    Q_OBJECT

public:
    JSInfoSystemHelper( JSPlugin* parent );
    ~JSInfoSystemHelper();

    QStringList requiredScriptPaths() const;

    Q_INVOKABLE void nativeAddInfoPlugin( int id );
    Q_INVOKABLE void nativeRemoveInfoPlugin( int id );

    Q_INVOKABLE void nativeAddInfoRequestResult( int infoPluginId, int requestId, int maxAge, const QVariantMap& returnedData );
    Q_INVOKABLE void nativeGetCachedInfo( int infoPluginId, int requestId, int newMaxAge, const QVariantMap& criteria);
    Q_INVOKABLE void nativeDataError( int infoPluginId, int requestId );


private:
    Q_DECLARE_PRIVATE( JSInfoSystemHelper )
    QScopedPointer<JSInfoSystemHelperPrivate> d_ptr;
};

} // ns: Tomahawk

#endif // TOMAHAWK_JSINFOSYSTEMHELPER_H
