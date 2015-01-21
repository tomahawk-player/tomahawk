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
#ifndef TOMAHAWK_SCRIPTACCOUNT_H
#define TOMAHAWK_SCRIPTACCOUNT_H

#include "../Typedefs.h"

#include <QObject>
#include <QVariantMap>

//TODO: pimple
#include "../utils/WeakObjectHash.h"
#include "ScriptCollection.h"
#include <QHash>
#include <QPixmap>

#include "../DllMacro.h"

namespace Tomahawk {

class ScriptObject;
class ScriptJob;
class ScriptCollectionFactory;
class ScriptInfoPluginFactory;

class DLLEXPORT ScriptAccount : public QObject
{
    Q_OBJECT

public:
    ScriptAccount( const QString& name );
    virtual ~ScriptAccount();

    void start();
    void stop();

    bool isStopped();

    QString name() const;

    void setIcon( const QPixmap& icon );
    QPixmap icon() const;

    void setFilePath( const QString& filePath );
    QString filePath() const;

    ScriptJob* invoke( const scriptobject_ptr& scriptObject, const QString& methodName, const QVariantMap& arguments );
    virtual QVariant syncInvoke( const scriptobject_ptr& scriptObject, const QString& methodName, const QVariantMap& arguments ) = 0;

    virtual void startJob( ScriptJob* scriptJob ) = 0;

    void reportScriptJobResult( const QVariantMap& result );
    void registerScriptPlugin( const QString& type, const QString& objectId );
    void unregisterScriptPlugin( const QString& type, const QString& objectId );

    virtual void scriptPluginFactory( const QString& type, const scriptobject_ptr& object );

    QList< Tomahawk::result_ptr > parseResultVariantList( const QVariantList& reslist );

    QSharedPointer< ScriptCollection > scriptCollection( const QString& id ) const;

private slots:
    void onJobDeleted( const QString& jobId );

    void onScriptObjectDeleted();

private: // TODO: pimple, might be renamed before tho
    QString m_name;
    QPixmap m_icon;
    QString m_filePath;
    bool m_stopped;
    QHash< QString, ScriptJob* > m_jobs;
    QHash< QString, scriptobject_ptr > m_objects;

    // port to QScopedPointer when pimple'd
    ScriptCollectionFactory* m_collectionFactory;
    ScriptInfoPluginFactory* m_infoPluginFactory;
};

} // ns: Tomahawk

#endif // TOMAHAWK_SCRIPTACCOUNT_H
