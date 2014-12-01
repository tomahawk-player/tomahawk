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

#ifndef TOMAHAWK_SCRIPTJOB_H
#define TOMAHAWK_SCRIPTJOB_H

#include <QVariantMap>
#include <QObject>

#include "DllMacro.h"

namespace Tomahawk
{

class ScriptObject;

class DLLEXPORT ScriptJob : public QObject
{
Q_OBJECT

public:
    ScriptJob( const QString& id, ScriptObject* scriptObject, const QString& methodName, const QVariantMap& arguments = QVariantMap() );
    virtual ~ScriptJob();

    virtual void start();

    const QString id() const;
    ScriptObject* scriptObject() const;
    const QString methodName() const;
    const QVariantMap arguments() const;

public slots:
    void reportResults( const QVariantMap& data );
    void reportFailure();

signals:
    void done( const QVariantMap& result );
    void failed( const QVariantMap& reason );

protected:
    // TODO: pimple
    QString m_id;
    ScriptObject* m_scriptObject;
    QVariantMap m_data;
    QString m_methodName;
    QVariantMap m_arguments;
};

}

#endif // TOMAHAWK_SCRIPTJOB_H
