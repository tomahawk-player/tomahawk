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

/*
    The collection  - acts as container for someones music library
    load() -> async populate by calling addArtists etc,
    then finishedLoading() is emitted.
    then use artists() etc to get the data.
*/

#ifndef TOMAHAWK_COLLECTION_H
#define TOMAHAWK_COLLECTION_H

#include <QHash>
#include <QDir>
#include <QList>
#include <QSharedPointer>

#include "Typedefs.h"
#include "Playlist.h"
#include "playlist/dynamic/DynamicPlaylist.h"

#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT Collection : public QObject
{
Q_OBJECT

public:
    Collection( const source_ptr& source, const QString& name, QObject* parent = 0 );
    virtual ~Collection();

    virtual QString name() const;

    virtual void loadPlaylists() { qDebug() << Q_FUNC_INFO; }
    virtual void loadAutoPlaylists() { qDebug() << Q_FUNC_INFO; }
    virtual void loadStations() { qDebug() << Q_FUNC_INFO; }

    virtual Tomahawk::playlist_ptr playlist( const QString& guid );
    virtual Tomahawk::dynplaylist_ptr autoPlaylist( const QString& guid );
    virtual Tomahawk::dynplaylist_ptr station( const QString& guid );

    virtual void addPlaylist( const Tomahawk::playlist_ptr& p );
    virtual void deletePlaylist( const Tomahawk::playlist_ptr& p );

    virtual void addAutoPlaylist( const Tomahawk::dynplaylist_ptr& p );
    virtual void deleteAutoPlaylist( const Tomahawk::dynplaylist_ptr& p );

    virtual void addStation( const Tomahawk::dynplaylist_ptr& s );
    virtual void deleteStation( const Tomahawk::dynplaylist_ptr& s );

    virtual QList< Tomahawk::playlist_ptr > playlists() { return m_playlists.values(); }
    virtual QList< Tomahawk::dynplaylist_ptr > autoPlaylists() { return m_autoplaylists.values(); }
    virtual QList< Tomahawk::dynplaylist_ptr > stations() { return m_stations.values(); }

    const source_ptr& source() const;
    unsigned int lastmodified() const { return m_lastmodified; }

signals:
    void tracksAdded( const QList<unsigned int>& fileids );
    void tracksRemoved( const QList<unsigned int>& fileids );

    void playlistsAdded( const QList<Tomahawk::playlist_ptr>& );
    void playlistsDeleted( const QList<Tomahawk::playlist_ptr>& );

    void autoPlaylistsAdded( const QList<Tomahawk::dynplaylist_ptr>& );
    void autoPlaylistsDeleted( const QList<Tomahawk::dynplaylist_ptr>& );

    void stationsAdded( const QList<Tomahawk::dynplaylist_ptr>& );
    void stationsDeleted( const QList<Tomahawk::dynplaylist_ptr>& );

    void changed();

public slots:
    virtual void addTracks( const QList<QVariant>& newitems ) = 0;
    virtual void removeTracks( const QDir& dir ) = 0;

    void setPlaylists( const QList<Tomahawk::playlist_ptr>& plists );
    void setAutoPlaylists( const QList< Tomahawk::dynplaylist_ptr >& autoplists );
    void setStations( const QList< Tomahawk::dynplaylist_ptr >& stations );

    void setTracks( const QList<unsigned int>& fileids );
    void delTracks( const QList<unsigned int>& fileids );

protected:
    QString m_name;
    unsigned int m_lastmodified; // unix time of last change to collection

private slots:
    void onSynced();

private:
    bool m_changed;

    source_ptr m_source;
    QHash< QString, Tomahawk::playlist_ptr > m_playlists;
    QHash< QString, Tomahawk::dynplaylist_ptr > m_autoplaylists;
    QHash< QString, Tomahawk::dynplaylist_ptr > m_stations;

    // HACK see line 99 in the dbcmd for why we need this for backwards-compatibility
    void moveAutoToStation( const QString& guid );
    void moveStationToAuto( const QString& guid );
    friend class ::DatabaseCommand_SetDynamicPlaylistRevision;
};

}; // ns

inline uint qHash( const QSharedPointer<Tomahawk::Collection>& key )
{
    return qHash( (void *)key.data() );
}

#endif // TOMAHAWK_COLLECTION_H
