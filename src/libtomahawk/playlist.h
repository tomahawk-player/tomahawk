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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>
#include <QList>
#include <QVariant>
#include <QSharedPointer>

#include "query.h"
#include "typedefs.h"

#include "playlistinterface.h"

#include "dllmacro.h"
#include <QQueue>

class DatabaseCommand_LoadAllPlaylists;
class DatabaseCommand_SetPlaylistRevision;
class DatabaseCommand_CreatePlaylist;

namespace Tomahawk
{

class DLLEXPORT PlaylistEntry : public QObject
{
Q_OBJECT
Q_PROPERTY( QString guid              READ guid         WRITE setGuid )
Q_PROPERTY( QString annotation        READ annotation   WRITE setAnnotation )
Q_PROPERTY( unsigned int duration     READ duration     WRITE setDuration )
Q_PROPERTY( unsigned int lastmodified READ lastmodified WRITE setLastmodified )
Q_PROPERTY( QVariant query            READ queryVariant WRITE setQueryVariant )

public:
    PlaylistEntry();
    virtual ~PlaylistEntry();

    void setQuery( const Tomahawk::query_ptr& q );
    const Tomahawk::query_ptr& query() const;

    // I wish Qt did this for me once i specified the Q_PROPERTIES:
    void setQueryVariant( const QVariant& v );
    QVariant queryVariant() const;

    QString guid() const { return m_guid; }
    void setGuid( const QString& s ) { m_guid = s; }

    QString annotation() const { return m_annotation; }
    void setAnnotation( const QString& s ) { m_annotation = s; }

    QString resultHint() const { return m_resulthint; }
    void setResultHint( const QString& s ) { m_resulthint= s; }

    unsigned int duration() const { return m_duration; }
    void setDuration( unsigned int i ) { m_duration = i; }

    unsigned int lastmodified() const { return m_lastmodified; }
    void setLastmodified( unsigned int i ) { m_lastmodified = i; }

    source_ptr lastSource() const;
    void setLastSource( source_ptr s );


private:
    QString m_guid;
    Tomahawk::query_ptr m_query;
    QString m_annotation;
    unsigned int m_duration;
    unsigned int m_lastmodified;
    source_ptr   m_lastsource;
    QString      m_resulthint;
};


struct PlaylistRevision
{
    QString revisionguid;
    QString oldrevisionguid;
    QList<plentry_ptr> newlist;
    QList<plentry_ptr> added;
    QList<plentry_ptr> removed;
    bool applied; // false if conflict
};

struct RevisionQueueItem
{
public:
    QString newRev;
    QString oldRev;
    QList< plentry_ptr > entries;
    bool applyToTip;

    RevisionQueueItem( const QString& nRev, const QString& oRev, const QList< plentry_ptr >& e, bool latest ) :
        newRev( nRev ), oldRev( oRev), entries( e ), applyToTip( latest ) {}
};


class DLLEXPORT Playlist : public QObject, public PlaylistInterface
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
friend class ::DatabaseCommand_SetPlaylistRevision;
friend class ::DatabaseCommand_CreatePlaylist;
friend class DynamicPlaylist;

public:
    ~Playlist();

    static Tomahawk::playlist_ptr load( const QString& guid );

    // one CTOR is private, only called by DatabaseCommand_LoadAllPlaylists
    static Tomahawk::playlist_ptr create( const source_ptr& author,
                                          const QString& guid,
                                          const QString& title,
                                          const QString& info,
                                          const QString& creator,
                                          bool shared,
                                          const QList<Tomahawk::query_ptr>& queries = QList<Tomahawk::query_ptr>() );

    static bool remove( const playlist_ptr& playlist );
    bool rename( const QString& title );

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

    const QList< plentry_ptr >& entries() { return m_entries; }
    virtual void addEntry( const Tomahawk::query_ptr& query, const QString& oldrev );
    virtual void addEntries( const QList<Tomahawk::query_ptr>& queries, const QString& oldrev );

    // <IGNORE hack="true">
    // these need to exist and be public for the json serialization stuff
    // you SHOULD NOT call them.  They are used for an alternate CTOR method from json.
    // maybe friend QObjectHelper and make them private?
    explicit Playlist( const source_ptr& author );
    void setCurrentrevision( const QString& s ) { m_currentrevision = s; }
    void setTitle( const QString& s )           { m_title = s; emit changed(); }
    void setInfo( const QString& s )            { m_info = s; }
    void setCreator( const QString& s )         { m_creator = s; }
    void setGuid( const QString& s )            { m_guid = s; }
    void setShared( bool b )                    { m_shared = b; }
    void setCreatedOn( uint createdOn )         { m_createdOn = createdOn; }
    // </IGNORE>

    virtual QList<Tomahawk::query_ptr> tracks();

    virtual int unfilteredTrackCount() const { return m_entries.count(); }
    virtual int trackCount() const { return m_entries.count(); }

    virtual bool hasNextItem() { return false; }
    virtual Tomahawk::result_ptr currentItem() const { return m_currentItem; }

    virtual Tomahawk::result_ptr siblingItem( int /*itemsAway*/ ) { return result_ptr(); }

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void setRepeatMode( PlaylistInterface::RepeatMode ) {}
    virtual void setShuffled( bool ) {}

    virtual void setFilter( const QString& /*pattern*/ ) {}

signals:
    /// emitted when the playlist revision changes (whenever the playlist changes)
    void revisionLoaded( Tomahawk::PlaylistRevision );

    /// watch for this to see when newly created playlist is synced to DB (if you care)
    void created();

    /// renamed etc.
    void changed();

    /**
     *   delete command is scheduled but not completed. Do not call remove() again once this
     *   is emitted.
     */
    void aboutToBeDeleted( const Tomahawk::playlist_ptr& pl );

    /// was deleted, eh?
    void deleted( const Tomahawk::playlist_ptr& pl );

    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

    void nextTrackReady();

public slots:
    // want to update the playlist from the model?
    // generate a newrev using uuid() and call this:
    void createNewRevision( const QString& newrev, const QString& oldrev, const QList< plentry_ptr >& entries );
    void reportCreated( const Tomahawk::playlist_ptr& self );
    void reportDeleted( const Tomahawk::playlist_ptr& self );

    void setRevision( const QString& rev,
                      const QList<QString>& neworderedguids,
                      const QList<QString>& oldorderedguids,
                      bool is_newest_rev,
                      const QMap< QString, Tomahawk::plentry_ptr >& addedmap,
                      bool applied );

    void resolve();

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

    QList<plentry_ptr> addEntriesInternal( const QList<Tomahawk::query_ptr>& queries );

private slots:
    void onResultsFound( const QList<Tomahawk::result_ptr>& results );
    void onResolvingFinished();

private:
    Playlist();
    void init();

    void setBusy( bool b );
    void checkRevisionQueue();

    source_ptr m_source;
    QString m_currentrevision;
    QString m_guid, m_title, m_info, m_creator;
    unsigned int m_lastmodified;
    unsigned int m_createdOn;
    bool m_shared;

    result_ptr m_currentItem;

    QList< plentry_ptr > m_initEntries;
    QList< plentry_ptr > m_entries;

    QQueue<RevisionQueueItem> m_revisionQueue;

    bool m_locallyChanged;
    bool m_busy;
};

};

Q_DECLARE_METATYPE( QSharedPointer< Tomahawk::Playlist > )

#endif // PLAYLIST_H
