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

#include "Query.h"
#include "Playlist.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include "DllMacro.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
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

    QUrl copyOpenLink( const Tomahawk::artist_ptr& artist ) const;
    QUrl copyOpenLink( const Tomahawk::album_ptr& album ) const;

    QUrl openLink( const QString& title, const QString& artist, const QString& album ) const;

public slots:
    void shortenLink( const QUrl& url, const QVariant &callbackObj = QVariant() );

#ifndef ENABLE_HEADLESS

    /// Takes a spotify link and performs the default open action on it
    bool openSpotifyLink( const QString& link );

    /// Takes a spotify link and performs the default open action on it
    bool openRdioLink( const QString& link );

    /// Creates a link from the requested data and copies it to the clipboard
    void copyToClipboard( const Tomahawk::query_ptr& query );


    QString copyPlaylistToClipboard( const Tomahawk::dynplaylist_ptr& playlist );
    void savePlaylistToFile( const Tomahawk::playlist_ptr& playlist, const QString& filename );

    bool parseTomahawkLink( const QString& link );
    void getShortLink( const Tomahawk::playlist_ptr& playlist );
    void waitingForResolved( bool );

    Tomahawk::dynplaylist_ptr loadDynamicPlaylist( const QUrl& url, bool station );

    void handleOpenTrack( const Tomahawk::query_ptr& qry );
    void handleOpenTracks( const QList< Tomahawk::query_ptr >& queries );

    void handlePlayTrack( const Tomahawk::query_ptr& qry );
#endif

signals:
    void shortLinkReady( const QUrl& longUrl, const QUrl& shortUrl, const QVariant& callbackObj );

private slots:
    void shortenLinkRequestFinished();
    void shortenLinkRequestError( QNetworkReply::NetworkError );

    void bookmarkPlaylistCreated( const Tomahawk::playlist_ptr& pl );

#ifndef ENABLE_HEADLESS
    void postShortenFinished();
    void showPlaylist();

    void playlistCreatedToShow( const Tomahawk::playlist_ptr& pl );
    void playlistReadyToShow();

    void xspfCreated( const QByteArray& xspf );

    void playOrQueueNow( const Tomahawk::query_ptr& );
    void playNow( const Tomahawk::query_ptr& );
#endif

private:
    explicit GlobalActionManager( QObject* parent = 0 );
    void doBookmark( const Tomahawk::playlist_ptr& pl, const Tomahawk::query_ptr& q );

    /// handle opening of urls
#ifndef ENABLE_HEADLESS
    bool handlePlaylistCommand( const QUrl& url );
    bool handleViewCommand( const QUrl& url );
    bool handleStationCommand( const QUrl& url );
    bool handleSearchCommand( const QUrl& url );
    bool handleQueueCommand( const QUrl& url );
    bool handleAutoPlaylistCommand( const QUrl& url );
    bool handleImportCommand( const QUrl& url );
    bool doQueueAdd( const QStringList& parts, const QList< QPair< QString, QString > >& queryItems );

    bool playSpotify( const QUrl& url );
    bool queueSpotify( const QStringList& parts, const QList< QPair< QString, QString > >& queryItems );
    bool playRdio( const QUrl& url );
    bool queueRdio( const QStringList& parts, const QList< QPair< QString, QString > >& queryItems );
#endif

    bool handleCollectionCommand( const QUrl& url );
    bool handlePlayCommand( const QUrl& url );
    bool handleBookmarkCommand( const QUrl& url );
    bool handleOpenCommand( const QUrl& url );

    void createPlaylistFromUrl( const QString& type, const QString& url, const QString& title );

    QString hostname() const;

    inline QByteArray percentEncode( const QUrl& url ) const;

    Tomahawk::playlist_ptr m_toShow;
    Tomahawk::query_ptr m_waitingToBookmark;
    Tomahawk::query_ptr m_waitingToPlay;
    QUrl m_clipboardLongUrl;

    static GlobalActionManager* s_instance;
};

#endif // GLOBALACTIONMANAGER_H
