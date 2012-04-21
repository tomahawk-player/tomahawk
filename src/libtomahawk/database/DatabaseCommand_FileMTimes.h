/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef DATABASECOMMAND_FILEMTIMES_H
#define DATABASECOMMAND_FILEMTIMES_H

#include <QObject>
#include <QVariantMap>
#include <QMap>
#include <QDir>

#include "DatabaseCommand.h"

#include "DllMacro.h"

// Not loggable, mtimes only used to speed up our local scanner.

class DLLEXPORT DatabaseCommand_FileMtimes : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_FileMtimes( const QString& prefix = QString(), QObject* parent = 0 )
        : DatabaseCommand( parent ), m_prefix( prefix ), m_checkonly( false )
    {}

    explicit DatabaseCommand_FileMtimes( const QStringList& prefixes, QObject* parent = 0 )
    : DatabaseCommand( parent ), m_prefixes( prefixes ), m_checkonly( false )
    {}

    //NOTE: when this is called we actually ignore the boolean flag; it's just used to give us the right constructor
    explicit DatabaseCommand_FileMtimes( bool /*checkonly*/, QObject* parent = 0 )
    : DatabaseCommand( parent ), m_checkonly( true )
    {}
    
    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "filemtimes"; }

signals:
    void done( const QMap< QString, QMap< unsigned int, unsigned int > >& );

public slots:

private:
    void execSelectPath( DatabaseImpl *dbi, const QDir& path, QMap< QString, QMap< unsigned int, unsigned int > > &mtimes );
    void execSelect( DatabaseImpl* dbi );
    QString m_prefix;
    QStringList m_prefixes;
    bool m_checkonly;
};

#endif // DATABASECOMMAND_FILEMTIMES_H
