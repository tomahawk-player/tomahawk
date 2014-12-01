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

#ifndef TOMAHAWK_SCRIPTOBJECT_H
#define TOMAHAWK_SCRIPTOBJECT_H

#include <QObject>
#include <QVariantMap>

#include "DllMacro.h"

namespace Tomahawk
{

class ScriptPlugin;
class ScriptObjectPrivate;
class ScriptJob;

class DLLEXPORT ScriptObject : public QObject
{
friend class JSPlugin;
friend class ScriptJob;

public:
    ScriptObject( ScriptPlugin* parent );
    virtual ~ScriptObject();

    ScriptJob* invoke( const QString& methodName, const QVariantMap& arguments );

protected:
    QString id() const;

    void startJob( ScriptJob* scriptJob );
    void removeJob( ScriptJob* scriptJob );
private:
    Q_DECLARE_PRIVATE( ScriptObject )
    QScopedPointer<ScriptObjectPrivate> d_ptr;
};

}; // ns: Tomahawk

#endif // TOMAHAWK_SCRIPTOBJECT_H
