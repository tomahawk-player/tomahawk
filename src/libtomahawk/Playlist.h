/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "Typedefs.h" // ModelMode
// #include "Result.h"
// #include "PlaylistEntry.h"
// #include "PlaylistInterface.h"
// #include "playlist/PlaylistUpdaterInterface.h"
// #include "Query.h"
#include "PlaylistRevision.h"
#include "utils/Closure.h"

#include "playlist_ptr.h"
#include "plentry_ptr.h"
#include "source_ptr.h"
#include "query_ptr.h"
#include "playlistinterface_ptr.h"

// #include <QObject>
// #include <QList>
// #include <QVariant>
// #include <QSharedPointer>
#include <QQueue>

#include "DllMacro.h"

class SourceTreePopupDialog;
class DatabaseCommand_LoadAllPlaylists;
class DatabaseCommand_LoadAllSortedPlaylists;
class DatabaseCommand_SetPlaylistRevision;
class DatabaseCommand_CreatePlaylist;
class PlaylistModel;

namespace Tomahawk
{

class PlaylistUpdaterInterface;
struct RevisionQueueItem;

class DLLEXPORT Playlist : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString guid            READ guid               WRITE setGuid )
    Q_PROPERTY( QString currentrevision READ currentrevision    WRITE setCurrentrevision )
    Q_PROPERTY( QString title           READ title              WRITE setTitle )
    Q_PROPERTY( QString info            READ info               WRITE setInfo )
    Q_PROPERTY( QString creator         READ creator            WRITE setCreator )
    Q_PROPERTY( unsigned int createdon  READ createdOn          WRITE setCreatedOn )
    Q_PROPERTY( bool    shared          READ shared             WRITE setShared )

friend class ::DatabaseCommand_LoadAllPlaylists;
friend class ::DatabaseCommand_LoadAllSortedPlaylists;
friend class ::DatabaseCommand_SetPlaylistRevision;
friend class ::DatabaseCommand_CreatePlaylist;
friend class DynamicPlaylist;
friend class ::PlaylistModel;

public:
    virtual ~Playlist();

    static Tomahawk::playlist_ptr get( const QString& guid );

    // one CTOR is private, only called by DatabaseCommand_LoadAllPlaylists
    static Tomahawk::playlist_ptr create( const source_ptr& author,
                                          const QString& guid,
                                          const QString& title,
                                          const QString& info,
                                          const QString& creator,
                                          bool shared,
                                          const QList<Tomahawk::query_ptr>& queries = QList<Tomahawk::query_ptr>() );

    static void remove( const playlist_ptr& playlist );
    void rename( const QString& title );

    virtual void loadRevision( const QString& rev = "" );

    source_ptr author() const;
    QString currentrevision() const   { return m_currentrevision; }
    QString title() const             { return m_title; }
    QString info() const              { return m_info; }
    QString creator() const           { return m_creator; }
    QString guid() const              { return m_guid; }
    bool shared() const               { return m_shared; }
    unsigned int lastmodified() const { return m_lastmodified; }
    uint createdOn() const            { return m_createdOn; }

    bool busy() const { return m_busy; }
    bool loaded() const { return m_loaded; }

    const QList< plentry_ptr >& entries() { return m_entries; }

    // <IGNORE hack="true">
    // these need to exist and be public for the json serialization stuff
    // you SHOULD NOT call them.  They are used for an alternate CTOR method from json.
    // maybe friend QObjectHelper and make them private?
    explicit Playlist( const source_ptr& author );
    void setCurrentrevision( const QString& s ) { m_currentrevision = s; }
    void setInfo( const QString& s )            { m_info = s; }
    void setCreator( const QString& s )         { m_creator = s; }
    void setGuid( const QString& s )            { m_guid = s; }
    void setShared( bool b )                    { m_shared = b; }
    void setCreatedOn( uint createdOn )         { m_createdOn = createdOn; }
    void setTitle( const QString& s );
    // </IGNORE>

    QList<plentry_ptr> entriesFromQueries( const QList<Tomahawk::query_ptr>& queries, bool clearFirst = false );

    void addUpdater( PlaylistUpdaterInterface* updater );
    void removeUpdater( PlaylistUpdaterInterface* updater );
    QList<PlaylistUpdaterInterface*> updaters() const { return m_updaters; }

    /**
     * Some updaters might have custom deleters in order to perform more actions that require
     * user prompting on delete.
     */
    bool hasCustomDeleter() const;
    /**
     * If this playlist has a custom deleter, let it do the deleting itself.
     *
     * If it needs user prompting, use the \param customDeleter as the right-most center point.
     */
    void customDelete( const QPoint& rightCenter );

    Tomahawk::playlistinterface_ptr playlistInterface();

signals:
    /// emitted when the playlist revision changes (whenever the playlist changes)
    void revisionLoaded( Tomahawk::PlaylistRevision );

    /// watch for this to see when newly created playlist is synced to DB (if you care)
    void created();

    /// renamed etc.
    void changed();
    void renamed( const QString& newTitle, const QString& oldTitle );

    /**
     *   delete command is scheduled but not completed. Do not call remove() again once this
     *   is emitted.
     */
    void aboutToBeDeleted( const Tomahawk::playlist_ptr& pl );

    /// was deleted, eh?
    void deleted( const Tomahawk::playlist_ptr& pl );

    /// Notification for tracks being inserted at a specific point
    /// Contiguous range from startPosition
    void tracksInserted( const QList< Tomahawk::plentry_ptr >& tracks, int startPosition );

    /// Notification for tracks being removed from playlist
    void tracksRemoved( const QList< Tomahawk::query_ptr >& tracks );

    /// Notification for tracks being moved in a playlist. List is of new tracks, and new position of first track
    /// Contiguous range from startPosition
    void tracksMoved( const QList< Tomahawk::plentry_ptr >& tracks, int startPosition );

public slots:
    // want to update the playlist from the model?
    // generate a newrev using uuid() and call this:
    void createNewRevision( const QString& newrev, const QString& oldrev, const QList< plentry_ptr >& entries );

    // Want to update some metadata of a plentry_ptr? this gets you a new revision too.
    // entries should be <= entries(), with changed metadata.
    void updateEntries( const QString& newrev, const QString& oldrev, const QList< plentry_ptr >& entries );

    virtual void addEntry( const Tomahawk::query_ptr& query );
    virtual void addEntries( const QList<Tomahawk::query_ptr>& queries );
    virtual void insertEntries( const QList<Tomahawk::query_ptr>& queries, const int position );

    void reportCreated( const Tomahawk::playlist_ptr& self );
    void reportDeleted( const Tomahawk::playlist_ptr& self );

    void setRevision( const QString& rev,
                      const QList<QString>& neworderedguids,
                      const QList<QString>& oldorderedguids,
                      bool is_newest_rev,
                      const QMap< QString, Tomahawk::plentry_ptr >& addedmap,
                      bool applied );

    void resolve();

    void setWeakSelf( QWeakPointer< Playlist > self );

protected:
    // called from loadAllPlaylists DB cmd:
    explicit Playlist( const source_ptr& src,
                       const QString& currentrevision,
                       const QString& title,
                       const QString& info,
                       const QString& creator,
                       uint createdOn,
                       bool shared,
                       int lastmod,
                       const QString& guid = "" ); // populate db

    // called when creating new playlist
    explicit Playlist( const source_ptr& author,
                       const QString& guid,
                       const QString& title,
                       const QString& info,
                       const QString& creator,
                       bool shared,
                       const QList< Tomahawk::plentry_ptr >& entries = QList< Tomahawk::plentry_ptr >() );

    QList< plentry_ptr > newEntries( const QList< plentry_ptr >& entries );
    PlaylistRevision setNewRevision( const QString& rev,
                                     const QList<QString>& neworderedguids,
                                     const QList<QString>& oldorderedguids,
                                     bool is_newest_rev,
                                     const QMap< QString, Tomahawk::plentry_ptr >& addedmap );

private slots:
    void onResultsChanged();
    void onResolvingFinished();

    void onDeleteResult( SourceTreePopupDialog* );

private:
    Playlist();
    void init();

    void setBusy( bool b );
    void setLoaded( bool b );
    void checkRevisionQueue();

    QWeakPointer< Playlist > m_weakSelf;
    source_ptr m_source;
    QString m_currentrevision;
    QString m_guid, m_title, m_info, m_creator;
    unsigned int m_lastmodified;
    unsigned int m_createdOn;
    bool m_shared;
    bool m_loaded;

    QQueue<_detail::Closure*> m_queuedOps;
    QList< plentry_ptr > m_initEntries;
    QList< plentry_ptr > m_entries;

    QQueue<RevisionQueueItem> m_revisionQueue;
    QQueue<RevisionQueueItem> m_updateQueue;

    QList<PlaylistUpdaterInterface*> m_updaters;

    bool m_locallyChanged;
    bool m_deleted;
    bool m_busy;

    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

}

Q_DECLARE_METATYPE( QSharedPointer< Tomahawk::Playlist > )
Q_DECLARE_METATYPE( QList< QSharedPointer< Tomahawk::Query > > )

#endif // PLAYLIST_H
