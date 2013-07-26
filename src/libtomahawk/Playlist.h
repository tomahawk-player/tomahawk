/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013     , Uwe L. Korn <uwelk@xhochy.com>
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

#include <QObject>
#include <QList>
#include <QVariant>
#include <QSharedPointer>
#include <QQueue>

#include "Typedefs.h"
#include "PlaylistEntry.h"
#include "Query.h"

#include "DllMacro.h"

class SourceTreePopupDialog;
class PlaylistModel;

namespace _detail
{
    class Closure;
}

namespace Tomahawk
{
class DatabaseCommand_LoadAllPlaylists;
class DatabaseCommand_LoadAllSortedPlaylists;
class DatabaseCommand_SetPlaylistRevision;
class DatabaseCommand_CreatePlaylist;

class Playlist;
class PlaylistPrivate;
class PlaylistUpdaterInterface;

struct PlaylistRevision
{
    QString revisionguid;
    QString oldrevisionguid;
    QList<plentry_ptr> newlist;
    QList<plentry_ptr> added;
    QList<plentry_ptr> removed;
    bool applied; // false if conflict
};

class DLLEXPORT PlaylistRemovalHandler : public QObject
{
Q_OBJECT

    friend class Playlist;

public slots:
    void remove( const playlist_ptr& playlist );

signals:
    void aboutToBeDeletePlaylist( const Tomahawk::playlist_ptr& playlist );

private:
    PlaylistRemovalHandler();
};


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

friend class DatabaseCommand_LoadAllPlaylists;
friend class DatabaseCommand_LoadAllSortedPlaylists;
friend class DatabaseCommand_SetPlaylistRevision;
friend class DatabaseCommand_CreatePlaylist;
friend class DynamicPlaylist;
friend class PlaylistRemovalHandler;
friend class ::PlaylistModel;

public:
    virtual ~Playlist();

    static QSharedPointer<PlaylistRemovalHandler> removalHandler();
    static Tomahawk::playlist_ptr get( const QString& guid );

    // one CTOR is private, only called by DatabaseCommand_LoadAllPlaylists
    static Tomahawk::playlist_ptr create( const source_ptr& author,
                                          const QString& guid,
                                          const QString& title,
                                          const QString& info,
                                          const QString& creator,
                                          bool shared,
                                          const QList<Tomahawk::query_ptr>& queries = QList<Tomahawk::query_ptr>() );

    void rename( const QString& title );

    virtual void loadRevision( const QString& rev = "" );

    source_ptr author() const;
    QString currentrevision() const;
    QString title() const;
    QString info() const;
    QString creator() const;
    QString guid() const;
    bool shared() const;
    unsigned int lastmodified() const;
    uint createdOn() const;

    bool busy() const;
    bool loaded() const;

    const QList< plentry_ptr >& entries();

    // <IGNORE hack="true">
    // these need to exist and be public for the json serialization stuff
    // you SHOULD NOT call them.  They are used for an alternate CTOR method from json.
    // maybe friend QObjectHelper and make them private?
    explicit Playlist( const source_ptr& author );
    void setCurrentrevision( const QString& s );
    void setInfo( const QString& s );
    void setCreator( const QString& s );
    void setGuid( const QString& s );
    void setShared( bool b );
    void setCreatedOn( uint createdOn );
    void setTitle( const QString& s );
    // </IGNORE>

    QList<plentry_ptr> entriesFromQueries( const QList<Tomahawk::query_ptr>& queries, bool clearFirst = false );

    void addUpdater( PlaylistUpdaterInterface* updater );
    void removeUpdater( PlaylistUpdaterInterface* updater );
    QList<PlaylistUpdaterInterface*> updaters() const;

    /**
     * Some updaters might have custom deleters in order to perform more actions that require
     * user prompting on delete.
     */
    bool hasCustomDeleter() const;

    Tomahawk::playlistinterface_ptr playlistInterface();

signals:
    /**
     * emitted when the playlist revision changes (whenever the playlist changes)
     */
    void revisionLoaded( Tomahawk::PlaylistRevision );

    /**
     * watch for this to see when newly created playlist is synced to DB (if you care)
     */
    void created();

    /// renamed etc.
    void changed();
    void renamed( const QString& newTitle, const QString& oldTitle );

    /**
     * Delete command is scheduled but not completed. Do not call remove() again once this
     * is emitted.
     */
    void aboutToBeDeleted( const Tomahawk::playlist_ptr& pl );

    /// was deleted, eh?
    void deleted( const Tomahawk::playlist_ptr& pl );

    /**
     * Notification for tracks being inserted at a specific point
     * Contiguous range from startPosition
     */
    void tracksInserted( const QList< Tomahawk::plentry_ptr >& tracks, int startPosition );

    /**
     * Notification for tracks being removed from PlaylistModel
     */
    void tracksRemoved( const QList< Tomahawk::query_ptr >& tracks );

    /**
     * Notification for tracks being moved in a playlist. List is of new tracks, and new position of first track
     * Contiguous range from startPosition
     */
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
                       const QList< Tomahawk::plentry_ptr >& entries );

    explicit Playlist( const source_ptr& author,
                       const QString& guid,
                       const QString& title,
                       const QString& info,
                       const QString& creator,
                       bool shared);

    QList< plentry_ptr > newEntries( const QList< plentry_ptr >& entries );
    PlaylistRevision setNewRevision( const QString& rev,
                                     const QList<QString>& neworderedguids,
                                     const QList<QString>& oldorderedguids,
                                     bool is_newest_rev,
                                     const QMap< QString, Tomahawk::plentry_ptr >& addedmap );

    virtual void removeFromDatabase();

    Playlist( PlaylistPrivate* d );

    Tomahawk::PlaylistPrivate* d_ptr;
private slots:
    void onResultsChanged();
    void onResolvingFinished();

private:
    Playlist();
    void init();

    void setBusy( bool b );
    void setLoaded( bool b );
    void checkRevisionQueue();

    Q_DECLARE_PRIVATE( Tomahawk::Playlist )
};

}

Q_DECLARE_METATYPE( QSharedPointer< Tomahawk::Playlist > )

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    // Qt5 automatically generated this Metatype
    Q_DECLARE_METATYPE( QList< QSharedPointer< Tomahawk::Query > > )
#endif

#endif // PLAYLIST_H
