/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
 *   Copyright (C) 2014  Dominik Schmidt <domme@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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

#pragma once
#ifndef TOMAHAWK_SCRIPTPLUGIN_H
#define TOMAHAWK_SCRIPTPLUGIN_H

#include <QObject>
#include <QVariantMap>

//TODO: pimple
#include <QHash>

#include "../DllMacro.h"

namespace Tomahawk {

class ScriptObject;
class ScriptJob;

class DLLEXPORT ScriptPlugin : public QObject
{
    Q_OBJECT

public:
    virtual ~ScriptPlugin() {}

    ScriptJob* invoke( ScriptObject* scriptObject, const QString& methodName, const QVariantMap& arguments );
    virtual void startJob( ScriptJob* scriptJob ) = 0;
    void removeJob( ScriptJob* );

    void reportScriptJobResult( const QVariantMap& result );
    void registerScriptPlugin( const QString& type, const QString& objectId );


private: // TODO: pimple, might be renamed before tho
    QHash< QString, ScriptJob* > m_jobs;
    QHash< QString, ScriptObject* > m_objects;
};

} // ns: Tomahawk

#endif // TOMAHAWK_SCRIPTPLUGIN_H
