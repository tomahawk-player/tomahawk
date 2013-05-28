/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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


#ifndef DATABASECOMMAND_DELETEINBOXENTRY_H
#define DATABASECOMMAND_DELETEINBOXENTRY_H

#include "DatabaseCommand.h"

#include "query_ptr.h"

class DatabaseCommand_DeleteInboxEntry : public DatabaseCommand
{
    Q_OBJECT
public:
    explicit DatabaseCommand_DeleteInboxEntry( const Tomahawk::query_ptr& query, QObject *parent = 0 );

    virtual void exec( DatabaseImpl* dbi );
    virtual bool doesMutates() const { return true; }
    virtual bool groupable() const { return true; }
    virtual bool localOnly() const { return true; }
    virtual QString commandname() const { return "deleteinboxentry"; }

signals:
    void done();

private:
    Tomahawk::query_ptr m_query;
};

#endif // DATABASECOMMAND_DELETEINBOXENTRY_H
