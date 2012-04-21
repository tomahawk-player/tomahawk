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

#ifndef DATABASECOMMAND_ADDSOURCE_H
#define DATABASECOMMAND_ADDSOURCE_H

#include <QObject>
#include <QVariantMap>

#include "DatabaseCommand.h"

#include "DllMacro.h"

class DLLEXPORT DatabaseCommand_addSource : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_addSource( const QString& username, const QString& fname, QObject* parent = 0 );
    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return true; }
    virtual QString commandname() const { return "addsource"; }

signals:
    void done( unsigned int, const QString& );

private:
    QString m_username, m_fname;
};

#endif // DATABASECOMMAND_ADDSOURCE_H
