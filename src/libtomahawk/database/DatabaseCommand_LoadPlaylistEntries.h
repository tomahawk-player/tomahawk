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

#ifndef DATABASECOMMAND_LOADPLAYLIST_H
#define DATABASECOMMAND_LOADPLAYLIST_H

// #include <QObject>
// #include <QVariantMap>

#include "DatabaseCommand.h"
// #include "Playlist.h"
#include "plentry_ptr.h"

#include <QStringList>

#include "DllMacro.h"

class DLLEXPORT DatabaseCommand_LoadPlaylistEntries : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_LoadPlaylistEntries( QString revision_guid, QObject* parent = 0 )
    : DatabaseCommand( parent ), m_islatest( true ), m_revguid( revision_guid )
    {}

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadplaylistentries"; }

    QString revisionGuid() const { return m_revguid; }

signals:
    void done( const QString& rev,
               const QList<QString>& orderedguid,
               const QList<QString>& oldorderedguid,
               bool islatest,
               const QMap< QString, Tomahawk::plentry_ptr >& added,
               bool applied );

protected:
    void generateEntries( DatabaseImpl* dbi );

    QStringList m_guids;
    QMap< QString, Tomahawk::plentry_ptr > m_entrymap;
    bool m_islatest;
    QStringList m_oldentries;

private:
    QString m_revguid;
};

#endif
