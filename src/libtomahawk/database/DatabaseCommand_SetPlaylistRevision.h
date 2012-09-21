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

#ifndef DATABASECOMMAND_SETPLAYLISTREVISION_H
#define DATABASECOMMAND_SETPLAYLISTREVISION_H

#include "DatabaseCommandLoggable.h"
#include "Playlist.h"
#include "qjson/qobjecthelper.h"

#include "DllMacro.h"

using namespace Tomahawk;

class DLLEXPORT DatabaseCommand_SetPlaylistRevision : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QString playlistguid      READ playlistguid  WRITE setPlaylistguid )
Q_PROPERTY( QString newrev            READ newrev        WRITE setNewrev )
Q_PROPERTY( QString oldrev            READ oldrev        WRITE setOldrev )
Q_PROPERTY( QVariantList orderedguids READ orderedguids  WRITE setOrderedguids )
Q_PROPERTY( QVariantList addedentries READ addedentriesV WRITE setAddedentriesV )
Q_PROPERTY( bool metadataUpdate       READ metadataUpdate WRITE setMetadataUpdate )

public:
    explicit DatabaseCommand_SetPlaylistRevision( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
        , m_applied( false )
        , m_localOnly( false )
        , m_metadataUpdate( false )
    {}

    // Constructor for inserting or removing entries
    DatabaseCommand_SetPlaylistRevision( const source_ptr& s,
                                                  const QString& playlistguid,
                                                  const QString& newrev,
                                                  const QString& oldrev,
                                                  const QStringList& orderedguids,
                                                  const QList<Tomahawk::plentry_ptr>& addedentries,
                                                  const QList<Tomahawk::plentry_ptr>& entries );

    // constructor for updating metadata only
    DatabaseCommand_SetPlaylistRevision( const source_ptr& s,
                                                  const QString& playlistguid,
                                                  const QString& newrev,
                                                  const QString& oldrev,
                                                  const QStringList& orderedguids,
                                                  const QList<Tomahawk::plentry_ptr>& entriesToUpdate );


    QString commandname() const { return "setplaylistrevision"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();

    virtual bool doesMutates() const { return true; }
    virtual bool localOnly() const { return m_localOnly; }
    virtual bool groupable() const { return true; }

    void setAddedentriesV( const QVariantList& vlist )
    {
        m_addedentries.clear();
        foreach( const QVariant& v, vlist )
        {
            PlaylistEntry* pep = new PlaylistEntry;
            QJson::QObjectHelper::qvariant2qobject( v.toMap(), pep );

            if ( pep->isValid() )
                m_addedentries << plentry_ptr( pep );
        }
    }

    QVariantList addedentriesV() const
    {
        QVariantList vlist;
        foreach( const plentry_ptr& pe, m_addedentries )
        {
            if ( !pe->isValid() )
                continue;

            QVariant v = QJson::QObjectHelper::qobject2qvariant( pe.data() );
            vlist << v;
        }
        return vlist;
    }

    void setPlaylistguid( const QString& s ) { m_playlistguid = s; }

    void setNewrev( const QString& s ) { m_newrev = s; }
    void setOldrev( const QString& s ) { m_oldrev = s; }
    QString newrev() const { return m_newrev; }
    QString oldrev() const { return m_oldrev; }
    QString playlistguid() const { return m_playlistguid; }
    bool metadataUpdate() const { return m_metadataUpdate; }
    void setMetadataUpdate( bool metadataUpdate ) { m_metadataUpdate = metadataUpdate; }

    void setOrderedguids( const QVariantList& l ) { m_orderedguids = l; }
    QVariantList orderedguids() const { return m_orderedguids; }

protected:
    bool m_applied;
    QStringList m_previous_rev_orderedguids;
    QString m_playlistguid;
    QString m_newrev, m_oldrev;
    QMap<QString, Tomahawk::plentry_ptr> m_addedmap;

    QString m_currentRevision;

private:
    QString hintFromQuery( const query_ptr& query ) const;

    QVariantList m_orderedguids;
    QList<Tomahawk::plentry_ptr> m_addedentries, m_entries;

    bool m_localOnly, m_metadataUpdate;
};

#endif // DATABASECOMMAND_SETPLAYLISTREVISION_H
