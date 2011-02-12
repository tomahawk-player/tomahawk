#ifndef ALBUMPROXYMODEL_H
#define ALBUMPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "playlistinterface.h"
#include "playlist/albummodel.h"

#include "dllmacro.h"

class DLLEXPORT AlbumProxyModel : public QSortFilterProxyModel, public PlaylistInterface
{
Q_OBJECT

public:
    explicit AlbumProxyModel( QObject* parent = 0 );

    virtual AlbumModel* sourceModel() const { return m_model; }
    virtual void setSourceModel( AlbumModel* sourceModel );

    virtual QList<Tomahawk::query_ptr> tracks() { Q_ASSERT( FALSE ); QList<Tomahawk::query_ptr> queries; return queries; }
    
    virtual int unfilteredTrackCount() const { return sourceModel()->rowCount( QModelIndex() ); }
    virtual int trackCount() const { return rowCount( QModelIndex() ); }
    virtual int albumCount() const { return rowCount( QModelIndex() ); }

    virtual void removeIndex( const QModelIndex& index );
    virtual void removeIndexes( const QList<QModelIndex>& indexes );

    virtual Tomahawk::result_ptr siblingItem( int direction );

    virtual void setFilter( const QString& pattern );

    virtual PlaylistInterface::RepeatMode repeatMode() const { return m_repeatMode; }
    virtual bool shuffled() const { return m_shuffled; }

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
    bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;

private:
    AlbumModel* m_model;
    RepeatMode m_repeatMode;
    bool m_shuffled;
};

#endif // ALBUMPROXYMODEL_H
