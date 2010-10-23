#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include <QAbstractItemModel>

#include "tomahawk/playlistinterface.h"

class QMetaData;

class TrackModel : public QAbstractItemModel, public PlaylistInterface
{
Q_OBJECT

public:
    explicit TrackModel( QObject* parent = 0 );
    virtual ~TrackModel();

    virtual QModelIndex index( int row, int column, const QModelIndex& parent ) const;
    virtual QModelIndex parent( const QModelIndex& child ) const;

    virtual int trackCount() const { return rowCount( QModelIndex() ); }

    virtual int rowCount( const QModelIndex& parent ) const;
    virtual int columnCount( const QModelIndex& parent ) const;

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual PlItem* previousItem() { return 0; }
    virtual PlItem* nextItem() { return 0; }
    virtual PlItem* siblingItem( int direction ) { return 0; }

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    virtual QPersistentModelIndex currentItem() { return m_currentIndex; }

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

signals:
    void trackCountChanged( unsigned int tracks );

public slots:
    virtual void setCurrentItem( const QModelIndex& index );

    virtual void setRepeatMode( PlaylistInterface::RepeatMode mode ) {}
    virtual void setShuffled( bool shuffled ) {}

private:
    QPersistentModelIndex m_currentIndex;
};

#endif // TRACKMODEL_H
