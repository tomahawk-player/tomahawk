#ifndef TRACKPROXYMODEL_H
#define TRACKPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "tomahawk/playlistinterface.h"
#include "playlist/trackmodel.h"

class TrackProxyModel : public QSortFilterProxyModel, public PlaylistInterface
{
Q_OBJECT

public:
    explicit TrackProxyModel ( QObject* parent = 0 );

    virtual TrackModel* model() const { return m_model; }
    virtual void setSourceModel( TrackModel* sourceModel );

    virtual QPersistentModelIndex currentItem() const { return mapFromSource( m_model->currentItem() ); }
    virtual void setCurrentItem( const QModelIndex& index ) { m_model->setCurrentItem( mapToSource( index ) ); }

    virtual int trackCount() const { return rowCount( QModelIndex() ); }

    virtual PlItem* previousItem();
    virtual PlItem* nextItem();
    virtual PlItem* siblingItem( int itemsAway );

    void setFilterRegExp( const QString& pattern );

    virtual PlaylistInterface::RepeatMode repeatMode() const { return m_repeatMode; }
    virtual bool shuffled() const { return m_shuffled; }

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );

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
};

#endif // TRACKPROXYMODEL_H
