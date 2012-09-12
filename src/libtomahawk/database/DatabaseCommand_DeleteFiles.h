/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef DATABASECOMMAND_DELETEFILES_H
#define DATABASECOMMAND_DELETEFILES_H

#include <QtCore/QObject>
#include <QtCore/QDir>
#include <QtCore/QVariantMap>

#include "database/DatabaseCommandLoggable.h"
#include "Typedefs.h"

#include "DllMacro.h"


class DLLEXPORT DatabaseCommand_DeleteFiles : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QVariantList ids READ ids WRITE setIds )
Q_PROPERTY( bool deleteAll READ deleteAll WRITE setDeleteAll )

public:
    explicit DatabaseCommand_DeleteFiles( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_DeleteFiles( const Tomahawk::source_ptr& source, QObject* parent = 0 )
    : DatabaseCommandLoggable( parent ), m_deleteAll( true )
    {
        setSource( source );
    }

    explicit DatabaseCommand_DeleteFiles( const QDir& dir, const Tomahawk::source_ptr& source, QObject* parent = 0 )
    : DatabaseCommandLoggable( parent ), m_dir( dir ), m_deleteAll( false )
    {
        setSource( source );
    }

    explicit DatabaseCommand_DeleteFiles( const QVariantList& ids, const Tomahawk::source_ptr& source, QObject* parent = 0 )
    : DatabaseCommandLoggable( parent ), m_ids( ids ), m_deleteAll( false )
    {
        setSource( source );
    }

    virtual QString commandname() const { return "deletefiles"; }

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return true; }
    virtual bool localOnly() const { return false; }
    virtual bool groupable() const { return true; }
    virtual void postCommitHook();

    QVariantList ids() const { return m_ids; }
    void setIds( const QVariantList& i ) { m_ids = i; }

    bool deleteAll() const { return m_deleteAll; }
    void setDeleteAll( const bool deleteAll ) { m_deleteAll = deleteAll; }

signals:
    void done( const QList<unsigned int>&, const Tomahawk::collection_ptr& );
    void notify( const QList<unsigned int>& ids );

private:
    QDir m_dir;
    QVariantList m_ids;
    QList<unsigned int> m_idList;
    bool m_deleteAll;
};

#endif // DATABASECOMMAND_DELETEFILES_H
