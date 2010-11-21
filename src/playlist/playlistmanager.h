#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QHash>
#include <QStackedWidget>

#include "tomahawk/collection.h"
#include "tomahawk/playlistinterface.h"

class CollectionModel;
class CollectionFlatModel;
class CollectionView;
class PlaylistView;
class TrackProxyModel;
class TrackModel;
class TrackView;
class SourceInfoWidget;

class PlaylistManager : public QObject
{
Q_OBJECT

public:
    explicit PlaylistManager( QObject* parent = 0 );
    ~PlaylistManager();

    QWidget* widget() const { return m_widget; }

    bool isSuperCollectionVisible() const { return true; }

//    QList<PlaylistView*> views( const Tomahawk::playlist_ptr& playlist ) { return m_views.value( playlist ); }

    bool show( const Tomahawk::playlist_ptr& playlist );
    bool show( const Tomahawk::album_ptr& album );
    bool show( const Tomahawk::collection_ptr& collection );
    bool show( const Tomahawk::source_ptr& source );
    bool showSuperCollection();

    void showCurrentTrack();

signals:
    void numSourcesChanged( unsigned int sources );
    void numTracksChanged( unsigned int tracks );
    void numArtistsChanged( unsigned int artists );
    void numShownChanged( unsigned int shown );

    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

public slots:
    void setTreeMode();
    void setTableMode();

    void setFilter( const QString& filter );

    void setRepeatMode( PlaylistInterface::RepeatMode mode );
    void setShuffled( bool enabled );

private slots:
    void applyFilter();
    void onTrackCountChanged( unsigned int );

private:
    void unlinkPlaylist();
    void linkPlaylist();

    QStackedWidget* m_widget;

    CollectionFlatModel* m_superCollectionFlatModel;
    CollectionView* m_superCollectionView;

    QList< Tomahawk::collection_ptr > m_superCollections;

    QHash< PlaylistInterface*, TrackView* > m_views;
    QHash< Tomahawk::collection_ptr, CollectionView* > m_collectionViews;
    QHash< Tomahawk::playlist_ptr, PlaylistView* > m_playlistViews;
    QHash< Tomahawk::album_ptr, PlaylistView* > m_albumViews;
    QHash< Tomahawk::source_ptr, SourceInfoWidget* > m_sourceViews;

    TrackProxyModel* m_currentProxyModel;
    TrackModel* m_currentModel;
    TrackView* m_currentView;

    QWidget* m_currentInfoWidget;

    int m_currentMode;
    bool m_superCollectionVisible;

    QTimer m_filterTimer;
    QString m_filter;
};

#endif // PLAYLISTMANAGER_H
