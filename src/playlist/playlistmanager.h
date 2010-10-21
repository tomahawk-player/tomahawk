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

class PlaylistManager : public QObject
{
Q_OBJECT

public:
    explicit PlaylistManager( QObject* parent = 0 );
    ~PlaylistManager();

    QWidget* widget() const { return m_widget; }

    bool isSuperCollectionVisible() const { return true; }
    QList< Tomahawk::collection_ptr > superCollections() const { return m_superCollections; }

    QList<PlaylistView*> views( const Tomahawk::playlist_ptr& playlist ) { return m_views.value( playlist ); }

    bool show( const Tomahawk::playlist_ptr& playlist );
    bool show( const Tomahawk::collection_ptr& collection );

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

    CollectionModel* m_superCollectionModel;
    CollectionFlatModel* m_superCollectionFlatModel;
    QList<CollectionView*> m_superCollectionViews;
    QList< Tomahawk::collection_ptr > m_superCollections;

    QHash< Tomahawk::playlist_ptr, QList<PlaylistView*> > m_views;
    TrackProxyModel* m_currentProxyModel;
    TrackModel* m_currentModel;

    int m_currentMode;
    bool m_superCollectionVisible;

    QTimer m_filterTimer;
    QString m_filter;
};

#endif // PLAYLISTMANAGER_H
