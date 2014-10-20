/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef VIEWMANAGER_H
#define VIEWMANAGER_H

#include "Artist.h"
#include "ViewPage.h"
#include "ViewPagePlugin.h"
#include "collection/Collection.h"
#include "PlaylistInterface.h"
#include "playlist/QueueView.h"

#include <QObject>
#include <QHash>
#include <QStackedWidget>

#include "DllMacro.h"

#include <functional>

class AlbumModel;
class GridView;
class AlbumInfoWidget;
class ArtistInfoWidget;
class CollectionModel;
class PlaylistViewPage;
class CollectionViewPage;
class PlaylistModel;
class TrackProxyModel;
class TrackModel;
class TreeProxyModel;
class TreeModel;
class TrackView;
class SourceInfoWidget;
class TrackInfoWidget;
class QPushButton;
class InboxModel;

namespace Tomahawk
{
    class DynamicWidget;
}

class DLLEXPORT ViewManager : public QObject
{
Q_OBJECT

public:
    static ViewManager* instance();

    explicit ViewManager( QObject* parent = 0 );
    virtual ~ViewManager();

    QWidget* widget() const { return m_widget; }

    QueueView* queue() const { return m_queue; }
    void setQueue( QueueView* queue ) { m_queue = queue; }

    Tomahawk::playlistinterface_ptr currentPlaylistInterface() const;
    Tomahawk::ViewPage* currentPage() const;
    Tomahawk::ViewPage* pageForInterface( Tomahawk::playlistinterface_ptr plInterface ) const;

    Tomahawk::ViewPage* show( Tomahawk::ViewPage* page );

    Tomahawk::ViewPage* inboxWidget() const;
    Tomahawk::ViewPage* dynamicPageWidget( const QString& pageName ) const;

    InboxModel* inboxModel();

    /// Get the view page for the given item. Not pretty...
    Tomahawk::ViewPage* pageForPlaylist( const Tomahawk::playlist_ptr& pl ) const;
    Tomahawk::ViewPage* pageForDynPlaylist( const Tomahawk::dynplaylist_ptr& pl ) const;

    /// Get a playlist (or dynamic playlist ) from a ViewPage* if the page is PlaylistViewPage or DynamicWidget.
    /// Lives here but used by SourcesModel
    Tomahawk::playlist_ptr playlistForPage( Tomahawk::ViewPage* ) const;

    // only use this is you need to create a playlist and show it directly and want it to be
    // linked to the sidebar. call it right after creating the playlist
    PlaylistViewPage* createPageForPlaylist( const Tomahawk::playlist_ptr& playlist );

    PlaylistViewPage* createPageForList( const QString& title, const QList< Tomahawk::query_ptr >& queries );

    void addDynamicPage( Tomahawk::ViewPagePlugin* viewPage, const QString& pageName = QString() );

signals:
    void playClicked();
    void pauseClicked();

    void tempPageActivated( Tomahawk::ViewPage* );
    void viewPageActivated( Tomahawk::ViewPage* );
    void viewPageAboutToBeDestroyed( Tomahawk::ViewPage* );
    void viewPageDestroyed();

    void historyBackAvailable( bool avail );
    void historyForwardAvailable( bool avail );

    void viewPageAdded( const QString& pageName, Tomahawk::ViewPage* page, int sortValue );

public slots:
    Tomahawk::ViewPage* showInboxPage();
    Tomahawk::ViewPage* showQueuePage();

//    void addDynamicPage( const QString& pageName, const QString& text, const QIcon& icon, function< Tomahawk::ViewPage*() > instanceLoader, int sortValue = 0 );
    Tomahawk::ViewPage* showDynamicPage( const QString& pageName );

    void showCurrentTrack();

    // Returns the shown viewpage
    Tomahawk::ViewPage* show( const Tomahawk::playlist_ptr& playlist );
    Tomahawk::ViewPage* show( const Tomahawk::dynplaylist_ptr& playlist );
    Tomahawk::ViewPage* show( const Tomahawk::artist_ptr& artist );
    Tomahawk::ViewPage* show( const Tomahawk::album_ptr& album );
    Tomahawk::ViewPage* show( const Tomahawk::query_ptr& query );
    Tomahawk::ViewPage* show( const Tomahawk::collection_ptr& collection );
    Tomahawk::ViewPage* show( const Tomahawk::source_ptr& source );

    void historyBack();
    void historyForward();

    QList< Tomahawk::ViewPage* > allPages() const;
    QList< Tomahawk::ViewPage* > historyPages() const;
    void destroyPage( Tomahawk::ViewPage* page );
    bool destroyCurrentPage();

    void playlistInterfaceChanged( Tomahawk::playlistinterface_ptr );

private slots:
    void onWidgetDestroyed( QWidget* widget );

private:
    void setPage( Tomahawk::ViewPage* page, bool trackHistory = true );

    Tomahawk::playlist_ptr playlistForInterface( Tomahawk::playlistinterface_ptr plInterface ) const;
    Tomahawk::dynplaylist_ptr dynamicPlaylistForInterface( Tomahawk::playlistinterface_ptr plInterface ) const;

    QWidget* m_widget;
    QStackedWidget* m_stack;

    QueueView* m_queue;
    Tomahawk::ViewPage* m_inboxWidget;
    InboxModel* m_inboxModel;

    QHash< QString, Tomahawk::ViewPage* > m_dynamicPages;
    QHash< QString, QPointer< Tomahawk::ViewPagePlugin > > m_dynamicPagePlugins;
    QHash< QString, std::function< Tomahawk::ViewPage*() > > m_dynamicPagesInstanceLoaders;

    QHash< Tomahawk::dynplaylist_ptr, QPointer<Tomahawk::DynamicWidget> > m_dynamicWidgets;
    QHash< Tomahawk::collection_ptr, QPointer<CollectionViewPage> > m_collectionViews;
    QHash< Tomahawk::artist_ptr, QPointer<ArtistInfoWidget> > m_artistViews;
    QHash< Tomahawk::album_ptr, QPointer<AlbumInfoWidget> > m_albumViews;
    QHash< Tomahawk::query_ptr, QPointer<TrackInfoWidget> > m_trackViews;
    QHash< Tomahawk::playlist_ptr, QPointer<PlaylistViewPage> > m_playlistViews;
    QHash< Tomahawk::source_ptr, QPointer<SourceInfoWidget> > m_sourceViews;

    QList<Tomahawk::ViewPage*> m_pageHistoryBack;
    QList<Tomahawk::ViewPage*> m_pageHistoryFwd;
    Tomahawk::ViewPage* m_currentPage;

    Tomahawk::collection_ptr m_currentCollection;

    static ViewManager* s_instance;
};

#endif // VIEWMANAGER_H
