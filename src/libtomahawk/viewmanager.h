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

#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QHash>
#include <QStackedWidget>

#include "collection.h"
#include "playlistinterface.h"
#include "viewpage.h"
#include "widgets/welcomewidget.h"

#include "dllmacro.h"

class AnimatedSplitter;
class AlbumModel;
class AlbumView;
class ArtistView;
class CollectionModel;
class CollectionFlatModel;
class CollectionView;
class PlaylistModel;
class PlaylistView;
class QueueView;
class TrackProxyModel;
class TrackModel;
class TreeProxyModel;
class TreeModel;
class TrackView;
class SourceInfoWidget;
class InfoBar;
class TopBar;
class WelcomeWidget;

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
    ~ViewManager();

    QWidget* widget() const { return m_widget; }
    PlaylistView* queue() const;
    TopBar* topbar() const { return m_topbar; }

    bool isSuperCollectionVisible() const;
    bool isNewPlaylistPageVisible() const;

    PlaylistInterface* currentPlaylistInterface() const;
    Tomahawk::ViewPage* currentPage() const;
    Tomahawk::ViewPage* pageForInterface( PlaylistInterface* interface ) const;
    int positionInHistory( Tomahawk::ViewPage* page ) const;

    Tomahawk::ViewPage* show( Tomahawk::ViewPage* page );

    Tomahawk::ViewPage* welcomeWidget() const { return m_welcomeWidget; }
    ArtistView* superCollectionView() const { return m_superCollectionView; }

    /// Get the view page for the given item. Not pretty...
    Tomahawk::ViewPage* pageForPlaylist( const Tomahawk::playlist_ptr& pl ) const;
    Tomahawk::ViewPage* pageForDynPlaylist( const Tomahawk::dynplaylist_ptr& pl ) const;
    Tomahawk::ViewPage* pageForCollection( const Tomahawk::collection_ptr& pl ) const;

    // only use this is you need to create a playlist and show it directly and want it to be
    // linked to the sidebar. call it right after creating the playlist
    PlaylistView* createPageForPlaylist( const Tomahawk::playlist_ptr& pl );

signals:
    void numSourcesChanged( unsigned int sources );
    void numTracksChanged( unsigned int tracks );
    void numArtistsChanged( unsigned int artists );
    void numShownChanged( unsigned int shown );

    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void statsAvailable( bool b );
    void modesAvailable( bool b );
    void filterAvailable( bool b );
    void modeChanged( PlaylistInterface::ViewMode mode );

    void playClicked();
    void pauseClicked();

    void historyBackAvailable( bool avail );
    void historyForwardAvailable( bool avail );

    void tempPageActivated( Tomahawk::ViewPage* );
    void viewPageActivated( Tomahawk::ViewPage* );

public slots:
    Tomahawk::ViewPage* showSuperCollection();
    Tomahawk::ViewPage* showWelcomePage();
    void showCurrentTrack();

    // Returns the shown viewpage
    Tomahawk::ViewPage* show( const Tomahawk::playlist_ptr& playlist );
    Tomahawk::ViewPage* show( const Tomahawk::dynplaylist_ptr& playlist );
    Tomahawk::ViewPage* show( const Tomahawk::artist_ptr& artist );
    Tomahawk::ViewPage* show( const Tomahawk::album_ptr& album );
    Tomahawk::ViewPage* show( const Tomahawk::collection_ptr& collection );
    Tomahawk::ViewPage* show( const Tomahawk::source_ptr& source );

    void historyBack();
    void historyForward();
    void showHistory( int historyPosition );

    void setTreeMode();
    void setTableMode();
    void setAlbumMode();

    void showQueue();
    void hideQueue();

    void setRepeatMode( PlaylistInterface::RepeatMode mode );
    void setShuffled( bool enabled );

    void playlistInterfaceChanged( PlaylistInterface* );

    // called by the playlist creation dbcmds
    void createPlaylist( const Tomahawk::source_ptr& src, const QVariant& contents );
    void createDynamicPlaylist( const Tomahawk::source_ptr& src, const QVariant& contents );

    // ugh need to set up the connection in tomahawk to libtomahawk
    void onPlayClicked();
    void onPauseClicked();

private slots:
    void setFilter( const QString& filter );
    void applyFilter();

    void onWidgetDestroyed( QWidget* widget );

private:
    void setHistoryPosition( int position );
    void setPage( Tomahawk::ViewPage* page, bool trackHistory = true );
    void updateView();
    void unlinkPlaylist();
    void saveCurrentPlaylistSettings();
    void loadCurrentPlaylistSettings();
    
    Tomahawk::playlist_ptr playlistForInterface( PlaylistInterface* interface ) const;
    Tomahawk::dynplaylist_ptr dynamicPlaylistForInterface( PlaylistInterface* interface ) const;
    Tomahawk::collection_ptr collectionForInterface( PlaylistInterface* interface ) const;

    QWidget* m_widget;
    InfoBar* m_infobar;
    TopBar* m_topbar;
    QStackedWidget* m_stack;
    AnimatedSplitter* m_splitter;

    PlaylistModel* m_queueModel;
    QueueView* m_queueView;

    AlbumModel* m_superAlbumModel;
    AlbumView* m_superAlbumView;
    TreeModel* m_superCollectionModel;
    ArtistView* m_superCollectionView;
    WelcomeWidget* m_welcomeWidget;

    QList< Tomahawk::collection_ptr > m_superCollections;

    QHash< Tomahawk::dynplaylist_ptr, Tomahawk::DynamicWidget* > m_dynamicWidgets;
    QHash< Tomahawk::collection_ptr, CollectionView* > m_collectionViews;
    QHash< Tomahawk::collection_ptr, ArtistView* > m_treeViews;
    QHash< Tomahawk::collection_ptr, AlbumView* > m_collectionAlbumViews;
    QHash< Tomahawk::artist_ptr, PlaylistView* > m_artistViews;
    QHash< Tomahawk::album_ptr, PlaylistView* > m_albumViews;
    QHash< Tomahawk::playlist_ptr, PlaylistView* > m_playlistViews;
    QHash< Tomahawk::source_ptr, SourceInfoWidget* > m_sourceViews;

    QList<Tomahawk::ViewPage*> m_pageHistory;
    int m_historyPosition;

    Tomahawk::collection_ptr m_currentCollection;
    int m_currentMode;

    QTimer m_filterTimer;
    QString m_filter;

    static ViewManager* s_instance;
};

#endif // PLAYLISTMANAGER_H
