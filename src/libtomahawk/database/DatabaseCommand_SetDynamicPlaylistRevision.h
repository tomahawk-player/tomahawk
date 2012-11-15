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

#ifndef DATABASECOMMAND_SETDYNAMICPLAYLISTREVISION_H
#define DATABASECOMMAND_SETDYNAMICPLAYLISTREVISION_H

#include "DatabaseCommand_SetPlaylistRevision.h"
#include "playlist/dynamic/GeneratorInterface.h"

using namespace Tomahawk;

class DatabaseCommand_SetDynamicPlaylistRevision : public DatabaseCommand_SetPlaylistRevision
{
    Q_OBJECT
    Q_PROPERTY( QString type                     READ type          WRITE setType )
    Q_PROPERTY( int           mode               READ mode          WRITE setMode )
    Q_PROPERTY( QVariantList controls            READ controlsV     WRITE setControlsV )

public:
    explicit DatabaseCommand_SetDynamicPlaylistRevision( QObject* parent = 0 )
    : DatabaseCommand_SetPlaylistRevision( parent )
    {}

    explicit DatabaseCommand_SetDynamicPlaylistRevision( const source_ptr& s,
                                                  const QString& playlistguid,
                                                  const QString& newrev,
                                                  const QString& oldrev,
                                                  const QStringList& orderedguids,
                                                  const QList<Tomahawk::plentry_ptr>& addedentries,
                                                  const QList<plentry_ptr>& entries,
                                                  const QString& type,
                                                  GeneratorMode mode,
                                                  const QList< dyncontrol_ptr >& controls );

    explicit DatabaseCommand_SetDynamicPlaylistRevision( const source_ptr& s,
                                                  const QString& playlistguid,
                                                  const QString& newrev,
                                                  const QString& oldrev,
                                                  const QString& type,
                                                  GeneratorMode mode,
                                                  const QList< dyncontrol_ptr >& controls );

    QString commandname() const { return "setdynamicplaylistrevision"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }
    virtual bool groupable() const { return true; }

    void setControlsV( const QVariantList& vlist )
    {
        m_controlsV = vlist;
    }

    QVariantList controlsV();

    QString type() const { return m_type; }
    int mode() const { return (int)m_mode; }

    void setType( const QString& type ) { m_type = type; }
    void setMode( int mode ) { m_mode = (GeneratorMode)mode; }

    void setPlaylist( DynamicPlaylist* pl ); // raw pointer b/c we don't have the shared pointer from inside the shared pointer

private:
    QString m_type;
    GeneratorMode m_mode;
    QList< dyncontrol_ptr > m_controls;
    QList< QVariant > m_controlsV;

    // ARG i hate sharedpointers sometimes
    DynamicPlaylist* m_playlist; // Only used if setting revision of a non-autoloaded playlist, as those aren't able to be looked up by guid
};

#endif // DATABASECOMMAND_SETDYNAMICPLAYLISTREVISION_H
