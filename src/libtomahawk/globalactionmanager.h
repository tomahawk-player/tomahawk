/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef GLOBALACTIONMANAGER_H
#define GLOBALACTIONMANAGER_H

#include "playlist.h"
#include "query.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include "dllmacro.h"

#include <QObject>
#include <QUrl>

/**
 * Handles global actions such as parsing and creation of links, mime data handling, etc
 */
class DLLEXPORT GlobalActionManager : public QObject
{
    Q_OBJECT
public:
    static GlobalActionManager* instance();
    virtual ~GlobalActionManager();

    QUrl openLinkFromQuery( const Tomahawk::query_ptr& query ) const;
    QUrl openLink( const QString& title, const QString& artist, const QString& album ) const;

    /// Takes a spotify link and performs the default open action on it
    bool openSpotifyLink( const QString& link );

    /// Takes a spotify link and performs the default open action on it
    bool openRdioLink( const QString& link );

    void copyToClipboard( const Tomahawk::query_ptr& query ) const;
    QString copyPlaylistToClipboard( const Tomahawk::dynplaylist_ptr& playlist );
    void savePlaylistToFile( const Tomahawk::playlist_ptr& playlist, const QString& filename );

    /**
     * QMimeData helpers
     *
     *  Call this to parse the tracks in a QMimeData object to query_ptrs. This will parse internal tomahawk
     *   data as well as all other formats supported (spotify, etc).
     *
     * Connect to tracks( QList< query_ptr> ); for the extracted tracks.
     */
    bool acceptsMimeData( const QMimeData* data, bool tracksOnly = true );
    void tracksFromMimeData( const QMimeData* data );
    QStringList mimeTypes() const;

public slots:
    bool parseTomahawkLink( const QString& link );
    void waitingForResolved( bool );

    Tomahawk::dynplaylist_ptr loadDynamicPlaylist( const QUrl& url, bool station );

    void handleOpenTrack( const Tomahawk::query_ptr& qry );
signals:
    /// QMimeData parsing results
    void tracks( const QList< Tomahawk::query_ptr >& tracks );

private slots:
    void bookmarkPlaylistCreated( const Tomahawk::playlist_ptr& pl );
    void showPlaylist();

    void xspfCreated( const QByteArray& xspf );
    void expandedUrls( QStringList );

    void spotifyToPlay( const Tomahawk::query_ptr& );
private:
    explicit GlobalActionManager( QObject* parent = 0 );
    void doBookmark( const Tomahawk::playlist_ptr& pl, const Tomahawk::query_ptr& q );

    /// handle opening of urls
    bool handlePlaylistCommand( const QUrl& url );
    bool handleCollectionCommand(const QUrl& url );
    bool handleQueueCommand(const QUrl& url );
    bool handleStationCommand(const QUrl& url );
    bool handleAutoPlaylistCommand(const QUrl& url );
    bool handleSearchCommand(const QUrl& url );
    bool handlePlayCommand(const QUrl& url );
    bool handleBookmarkCommand(const QUrl& url );
    bool handleOpenCommand(const QUrl& url );
    bool doQueueAdd( const QStringList& parts, const QList< QPair< QString, QString > >& queryItems );

    bool playSpotify( const QUrl& url );
    bool queueSpotify( const QStringList& parts, const QList< QPair< QString, QString > >& queryItems );

    /// handle parsing mime data
    void handleTrackUrls( const QString& urls );
    QList< Tomahawk::query_ptr > tracksFromQueryList( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromResultList( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromArtistMetaData( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromAlbumMetaData( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromMixedData( const QMimeData* d );

    QString hostname() const;

    Tomahawk::playlist_ptr m_toShow;
    Tomahawk::query_ptr m_waitingToBookmark;
    Tomahawk::query_ptr m_waitingToPlay;

    QStringList m_mimeTypes;
    static GlobalActionManager* s_instance;
};

#endif // GLOBALACTIONMANAGER_H
