/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef DATABASECOMMAND_NETWORKCHARTS_H
#define DATABASECOMMAND_NETWORKCHARTS_H

#include <QObject>
#include <QDateTime>
#include <QVariantMap>

#include "Typedefs.h"
#include "DatabaseCommand.h"

#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT DatabaseCommand_NetworkCharts : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_NetworkCharts( QObject* parent = 0 );
    explicit DatabaseCommand_NetworkCharts( const QDateTime& from, const QDateTime& to, QObject* parent = 0 );
    virtual ~DatabaseCommand_NetworkCharts();

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "networkcharts"; }

    void setLimit( unsigned int amount ) { m_amount = amount; }

signals:
    void done( const QList<Tomahawk::track_ptr>& tracks );

private:
    unsigned int m_amount;
    QDateTime m_from;
    QDateTime m_to;
};

}

#endif // DATABASECOMMAND_NETWORKCHARTS_H
