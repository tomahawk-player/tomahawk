/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include <QObject>
#include <QDir>
#include <QVariantMap>

#include "database/databasecommandloggable.h"
#include "typedefs.h"
#include "query.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_DeleteFiles : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QVariantList ids READ ids WRITE setIds )

public:
    explicit DatabaseCommand_DeleteFiles( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_DeleteFiles( const QDir& dir, const Tomahawk::source_ptr& source, QObject* parent = 0 )
    : DatabaseCommandLoggable( parent ), m_dir( dir )
    {
        setSource( source );
    }

    explicit DatabaseCommand_DeleteFiles( const QVariantList& ids, const Tomahawk::source_ptr& source, QObject* parent = 0 )
    : DatabaseCommandLoggable( parent ), m_ids( ids )
    {
        setSource( source );
    }

    virtual QString commandname() const { return "deletefiles"; }

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return true; }
    virtual bool localOnly() const { return m_files.isEmpty(); }
    virtual void postCommitHook();

    QStringList files() const { return m_files; }
    void setFiles( const QStringList& f ) { m_files = f; }

    QVariantList ids() const { return m_ids; }
    void setIds( const QVariantList& i ) { m_ids = i; }

signals:
    void done( const QStringList&, const Tomahawk::collection_ptr& );
    void notify( const QStringList& );

private:
    QDir m_dir;
    QStringList m_files;
    QVariantList m_ids;
};

#endif // DATABASECOMMAND_DELETEFILES_H
