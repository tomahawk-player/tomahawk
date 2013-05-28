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

#ifndef DATABASECOMMAND_ADDFILES_H
#define DATABASECOMMAND_ADDFILES_H

#include "database/DatabaseCommandLoggable.h"
// #include "Typedefs.h"
// #include "Query.h"
#include "collection_ptr.h"

// #include <QObject>
// #include <QVariantMap>

#include "DllMacro.h"

class DLLEXPORT DatabaseCommand_AddFiles : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QVariantList files READ files WRITE setFiles )

public:
    explicit DatabaseCommand_AddFiles( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_AddFiles( const QList<QVariant>& files, const Tomahawk::source_ptr& source, QObject* parent = 0 )
        : DatabaseCommandLoggable( parent ), m_files( files )
    {
        setSource( source );
    }

    virtual QString commandname() const { return "addfiles"; }

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return true; }
    virtual void postCommitHook();

    QVariantList files() const;
    void setFiles( const QVariantList& f ) { m_files = f; }

signals:
    void done( const QList<QVariant>&, const Tomahawk::collection_ptr& );
    void notify( const QList<unsigned int>& ids );

private:
    QVariantList m_files;
    QList<unsigned int> m_ids;
};

#endif // DATABASECOMMAND_ADDFILES_H
