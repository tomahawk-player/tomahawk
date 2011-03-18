#ifndef TRACKPROXYMODEL_H
#define TRACKPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "playlistinterface.h"
#include "playlist/trackmodel.h"

#include "dllmacro.h"

class DLLEXPORT TrackProxyModel : public QSortFilterProxyModel, public PlaylistInterface
{
Q_OBJECT

public:
    explicit TrackProxyModel ( QObject* parent = 0 );

    virtual TrackModel* sourceModel() const { return m_model; }
    virtual void setSourceModel( TrackModel* sourceModel );

    virtual QPersistentModelIndex currentItem() const { return mapFromSource( m_model->currentItem() ); }
    virtual void setCurrentItem( const QModelIndex& index ) { m_model->setCurrentItem( mapToSource( index ) ); }

    virtual QList<Tomahawk::query_ptr> tracks();

    virtual int unfilteredTrackCount() const { return sourceModel()->trackCount(); }
    virtual int trackCount() const { return rowCount( QModelIndex() ); }

    virtual void removeIndex( const QModelIndex& index );
    virtual void removeIndexes( const QModelIndexList& indexes );
    virtual void removeIndexes( const QList<QPersistentModelIndex>& indexes );

    virtual Tomahawk::result_ptr siblingItem( int itemsAway );

    virtual QString filter() const { return filterRegExp().pattern(); }
    virtual void setFilter( const QString& pattern );

    virtual PlaylistInterface::RepeatMode repeatMode() const { return m_repeatMode; }
    virtual bool shuffled() const { return m_shuffled; }

    bool showOfflineResults() const { return m_showOfflineResults; }
    void setShowOfflineResults( bool b ) { m_showOfflineResults = b; }

    PlItem* itemFromIndex( const QModelIndex& index ) const { return sourceModel()->itemFromIndex( index ); }

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

    void filterChanged( const QString& filter );

public slots:
    virtual void setRepeatMode( RepeatMode mode ) { m_repeatMode = mode; emit repeatModeChanged( mode ); }
    virtual void setShuffled( bool enabled ) { m_shuffled = enabled; emit shuffleModeChanged( enabled ); }

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;

private:
    TrackModel* m_model;
    RepeatMode m_repeatMode;
    bool m_shuffled;
    bool m_showOfflineResults;
};

#endif // TRACKPROXYMODEL_H
