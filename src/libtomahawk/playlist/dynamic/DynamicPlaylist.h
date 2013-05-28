/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef DYNAMIC_PLAYLIST_H
#define DYNAMIC_PLAYLIST_H

// #include "Typedefs.h"
#include "RevisionQueueItem.h"
#include "DynamicPlaylistRevision.h"
#include "Playlist.h"
// #include "playlist/dynamic/DynamicControl.h"
// #include <QObject>
// #include <QList>
// #include <QSharedPointer>
//
#include "dyncontrol_ptr.h"
#include "dynplaylist_ptr.h"
#include "geninterface_ptr.h"


#include "DllMacro.h"

class DatabaseCommand_LoadAllDynamicPlaylists;
class DatabaseCommand_SetDynamicPlaylistRevision;
class DatabaseCommand_CreateDynamicPlaylist;
class DatabaseCommand_LoadAllSortedPlaylists;
class DatabaseCollection;

namespace Tomahawk {

class DatabaseCommand_LoadDynamicPlaylist;

/**
 * Subclass of playlist that adds the information needed to store a dynamic playlist.
 *  It uses normal PlaylistEntries but also has a mode, a generator, and a list of controls
*/

struct DynQueueItem : RevisionQueueItem
{
    QString type;
    QList <dyncontrol_ptr> controls;
    int mode;

    DynQueueItem( const QString& nRev, const QString& oRev, const QString& typ, const QList< dyncontrol_ptr >& ctrls,  int m, const QList< plentry_ptr >& e, bool latest ) :
        RevisionQueueItem( nRev, oRev, e, latest ), type( typ ), controls( ctrls ), mode( m ) {}
};

class DLLEXPORT DynamicPlaylist : public Tomahawk::Playlist
{
    Q_OBJECT

    // :-( int becuase qjson chokes on my enums
    Q_PROPERTY( int     mode                  WRITE setMode   READ mode )
    Q_PROPERTY( QString type                  WRITE setType   READ type )
    Q_PROPERTY( bool    autoLoad                              READ autoLoad )

    friend class ::DatabaseCommand_SetDynamicPlaylistRevision;
    friend class ::DatabaseCommand_CreateDynamicPlaylist;
    friend class Tomahawk::DatabaseCommand_LoadDynamicPlaylist;
    friend class ::DatabaseCommand_LoadAllSortedPlaylists;
    friend class ::DatabaseCollection; /// :-(

public:
    virtual ~DynamicPlaylist();

    static Tomahawk::dynplaylist_ptr get( const QString& guid );

    /// Generate an empty dynamic playlist with default generator
    static Tomahawk::dynplaylist_ptr create( const source_ptr& author,
                                             const QString& guid,
                                             const QString& title,
                                             const QString& info,
                                             const QString& creator,
                                             GeneratorMode mode,
                                             bool shared,
                                             const QString& type = QString(),
                                             bool autoLoad = true
                                           );

    static void remove( const dynplaylist_ptr& playlist );
    virtual void loadRevision( const QString& rev = "" );

    // :-( int becuase qjson chokes on my enums
    int mode() const;
    QString type() const;
    geninterface_ptr generator() const;
    bool autoLoad() const  { return m_autoLoad; }

    // Creates a new revision from the playlist in memory. Use this is you change the controls or
    // mode of a playlist and want to save it to db/others.
    void createNewRevision( const QString& uuid = QString() );

    virtual void addEntries( const QList< query_ptr >& queries );
    virtual void addEntry( const Tomahawk::query_ptr& query );

    // <IGNORE hack="true">
    // these need to exist and be public for the json serialization stuff
    // you SHOULD NOT call them.  They are used for an alternate CTOR method from json.
    // maybe friend QObjectHelper and make them private?
    explicit DynamicPlaylist( const source_ptr& author, const QString& type );
    void setMode( int mode );
    void setType( const QString& /*type*/ ) { /** TODO */; }
    void setGenerator( const geninterface_ptr& gen_ptr );
    // </IGNORE>

signals:
    /// emitted when the playlist revision changes (whenever the playlist changes)
    void dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision );

    void aboutToBeDeleted( const Tomahawk::dynplaylist_ptr& pl );
    void deleted( const Tomahawk::dynplaylist_ptr& pl );

public slots:
    // want to update the playlist from the model?
    // generate a newrev using uuid() and call this:
    // if this is a static playlist, pass it a new list of entries. implicitly sets mode to static
    void createNewRevision( const QString& newrev, const QString& oldrev, const QString& type, const QList< dyncontrol_ptr>& controls, const QList< plentry_ptr >& entries );
    // if it is ondemand, no entries are needed implicitly sets mode to ondemand
    void createNewRevision( const QString& newrev, const QString& oldrev, const QString& type, const QList< dyncontrol_ptr>& controls );

    void reportCreated( const Tomahawk::dynplaylist_ptr& self );
    void reportDeleted( const Tomahawk::dynplaylist_ptr& self );

    // called from setdynamicplaylistrevision db cmd
    // 4 options, because dbcmds can't create qwidgets:
    // static version,   qvariant controls
    // static version,   dyncontrol_ptr controls
    // ondemand version, qvariant controls
    // ondemand version, dyncontrol_ptr controls
    void setRevision( const QString& rev,
                      const QList<QString>& neworderedguids,
                      const QList<QString>& oldorderedguids,
                      const QString& type,
                      const QList< QVariantMap >& controls,
                      bool is_newest_rev,
                      const QMap< QString, Tomahawk::plentry_ptr >& addedmap,
                      bool applied );
    void setRevision( const QString& rev,
                      const QList<QString>& neworderedguids,
                      const QList<QString>& oldorderedguids,
                      const QString& type,
                      const QList< Tomahawk::dyncontrol_ptr >& controls,
                      bool is_newest_rev,
                      const QMap< QString, Tomahawk::plentry_ptr >& addedmap,
                      bool applied );
    // ondemand version
    void setRevision( const QString& rev,
                      bool is_newest_rev,
                      const QString& type,
                      const QList< QVariantMap>& controls,
                      bool applied );
    void setRevision( const QString& rev,
                      bool is_newest_rev,
                      const QString& type,
                      const QList< Tomahawk::dyncontrol_ptr>& controls,
                      bool applied );
private:
    // called from loadAllPlaylists DB cmd via databasecollection (in GUI thread)
    explicit DynamicPlaylist( const source_ptr& src,
                              const QString& currentrevision,
                              const QString& title,
                              const QString& info,
                              const QString& creator,
                              uint createdOn,
                              const QString& type,
                              GeneratorMode mode,
                              bool shared,
                              int lastmod,
                              const QString& guid = "" ); // populate db

    // called when creating new playlist
    explicit DynamicPlaylist( const source_ptr& author,
                              const QString& guid,
                              const QString& title,
                              const QString& info,
                              const QString& creator,
                              const QString& type,
                              GeneratorMode mode,
                              bool shared,
                              bool autoLoad = true );

    void checkRevisionQueue();

    QList< dyncontrol_ptr > variantsToControl( const QList< QVariantMap >& controlsV );

    geninterface_ptr m_generator;
    bool m_autoLoad;

    QQueue<DynQueueItem> m_revisionQueue;
};

}; // namespace

Q_DECLARE_METATYPE( QSharedPointer< Tomahawk::DynamicPlaylist > )

#endif
