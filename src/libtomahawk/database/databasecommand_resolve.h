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

#ifndef DATABASECOMMAND_RESOLVE_H
#define DATABASECOMMAND_RESOLVE_H

#include "databasecommand.h"
#include "databaseimpl.h"
#include "result.h"

#include <QVariant>

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_Resolve : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_Resolve( const Tomahawk::query_ptr& query );

    virtual QString commandname() const { return "dbresolve"; }
    virtual bool doesMutates() const { return false; }

    virtual void exec( DatabaseImpl *lib );

signals:
    void results( Tomahawk::QID qid, QList<Tomahawk::result_ptr> results );

public slots:

private:
    Tomahawk::query_ptr m_query;

    float how_similar( const Tomahawk::query_ptr& q, const Tomahawk::result_ptr& r );
    static int levenshtein( const QString& source, const QString& target );
};

#endif // DATABASECOMMAND_RESOLVE_H
