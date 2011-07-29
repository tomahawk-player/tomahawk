/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef DATABASECOMMAND_GENERICSELECT_H
#define DATABASECOMMAND_GENERICSELECT_H

#include <QVariantMap>

#include "databasecommand.h"
#include "source.h"
#include "typedefs.h"

#include "dllmacro.h"

/**
 * This dbcmd takes a generic SELECT command that operates on the database and returns a list of query_ptrs
 *  that match.
 *
 * In order for the conversion to query_ptr to work, the SELECT command should select the following items:
 * track.name, artist.name, album.name
 *
 */
class DLLEXPORT DatabaseCommand_GenericSelect : public DatabaseCommand
{
    Q_OBJECT

public:
    explicit DatabaseCommand_GenericSelect( const QString& sqlSelect, QObject* parent = 0 );
    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return false; }

    virtual QString commandname() const { return "genericselect"; }

signals:
    void tracks( const QList< Tomahawk::query_ptr >& tracks );

private:
    QString m_sqlSelect;
};

#endif // DATABASECOMMAND_GENERICSELECT_H
