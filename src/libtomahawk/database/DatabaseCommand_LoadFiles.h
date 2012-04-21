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

#ifndef DATABASECOMMAND_LOADFILES_H
#define DATABASECOMMAND_LOADFILES_H

#include <QObject>
#include <QVariantMap>
#include <QStringList>
#include <QMap>

#include "DatabaseCommand.h"
#include "Result.h"
#include "DllMacro.h"

/**
  Loads a result_ptr from the database from a track dbid.

  If use use the QStringList constructor, isten to results() instead of result()
  */
class DLLEXPORT DatabaseCommand_LoadFiles : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_LoadFiles( unsigned int id, QObject* parent = 0 );
    explicit DatabaseCommand_LoadFiles( const QList<unsigned int>& ids, QObject* parent = 0 );

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadfiles"; }

signals:
    void result( const Tomahawk::result_ptr& result );
    void results( const QList<Tomahawk::result_ptr>& results );

private:
    bool m_single;
    QList<unsigned int> m_ids;
};

#endif // DATABASECOMMAND_LOADFILE_H
