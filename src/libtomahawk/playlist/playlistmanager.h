#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QHash>
#include <QStackedWidget>

#include "collection.h"
#include "playlistinterface.h"
#include "viewpage.h"

#include "dllmacro.h"

class AnimatedSplitter;
class AlbumModel;
class AlbumView;
class CollectionModel;
class CollectionFlatModel;
class CollectionView;
class PlaylistModel;
class PlaylistView;
class QueueView;
class TrackProxyModel;
class TrackModel;
class TrackView;
class SourceInfoWidget;
class InfoBar;
class TopBar;
class WelcomeWidget;

namespace Tomahawk
{
    class DynamicWidget;
}

class DLLEXPORT PlaylistManager : public QObject
{
Q_OBJECT

public:
    static PlaylistManager* instance();

    explicit PlaylistManager( QObject* parent = 0 );
    ~PlaylistManager();

    QWidget* widget() const { return m_widget; }
    PlaylistView* queue() const;

    bool isSuperCollectionVisible() const;

    PlaylistInterface* currentPlaylistInterface() const;
    Tomahawk::ViewPage* currentPage() const;
    Tomahawk::ViewPage* pageForInterface( PlaylistInterface* interface ) const;
    int positionInHistory( Tomahawk::ViewPage* page ) const;

    bool show( const Tomahawk::playlist_ptr& playlist );
    bool show( const Tomahawk::dynplaylist_ptr& playlist );
    bool show( const Tomahawk::artist_ptr& artist );
    bool show( const Tomahawk::album_ptr& album );
    bool show( const Tomahawk::collection_ptr& collection );
    bool show( const Tomahawk::source_ptr& source );

    bool show( Tomahawk::ViewPage* page );

signals:
    void numSourcesChanged( unsigned int sources );
    void numTracksChanged( unsigned int tracks );
    void numArtistsChanged( unsigned int artists );
    void numShownChanged( unsigned int shown );

    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void statsAvailable( bool b );
    void modesAvailable( bool b );
    void modeChanged( PlaylistInterface::ViewMode mode );

    void playClicked();
    void pauseClicked();

    void historyBackAvailable( bool avail );
    void historyForwardAvailable( bool avail );

    void tempPageActivated();
    void superCollectionActivated();
    void collectionActivated( const Tomahawk::collection_ptr& collection );
    void playlistActivated( const Tomahawk::playlist_ptr& playlist );
    void dynamicPlaylistActivated( const Tomahawk::dynplaylist_ptr& playlist );
    
public slots:
    bool showSuperCollection();
    void showWelcomePage();
    void showCurrentTrack();

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
    CollectionFlatModel* m_superCollectionFlatModel;
    CollectionView* m_superCollectionView;
    WelcomeWidget* m_welcomeWidget;
    
    QList< Tomahawk::collection_ptr > m_superCollections;

    QHash< Tomahawk::dynplaylist_ptr, Tomahawk::DynamicWidget* > m_dynamicWidgets;
    QHash< Tomahawk::collection_ptr, CollectionView* > m_collectionViews;
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

    static PlaylistManager* s_instance;
};

#endif // PLAYLISTMANAGER_H
