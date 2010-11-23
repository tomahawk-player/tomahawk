#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include <QAbstractItemModel>

#include "tomahawk/playlistinterface.h"
#include "playlist/plitem.h"

class QMetaData;

class TrackModel : public QAbstractItemModel, public PlaylistInterface
{
Q_OBJECT

public:
    enum Columns {
        Artist = 0,
        Track = 1,
        Album = 2,
        Duration = 3,
        Bitrate = 4,
        Age = 5,
        Origin = 6
    };

    explicit TrackModel( QObject* parent = 0 );
    virtual ~TrackModel();

    virtual QModelIndex index( int row, int column, const QModelIndex& parent ) const;
    virtual QModelIndex parent( const QModelIndex& child ) const;

    virtual bool isReadOnly() const { return m_readOnly; }

    virtual int trackCount() const { return rowCount( QModelIndex() ); }

    virtual int rowCount( const QModelIndex& parent ) const;
    virtual int columnCount( const QModelIndex& parent ) const;

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual void removeIndex( const QModelIndex& index );
    virtual void removeIndexes( const QList<QModelIndex>& indexes );

    virtual Tomahawk::result_ptr siblingItem( int direction ) { return Tomahawk::result_ptr(); }

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    virtual QPersistentModelIndex currentItem() { return m_currentIndex; }

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    PlItem* itemFromIndex( const QModelIndex& index ) const;

    PlItem* m_rootItem;

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );

public slots:
    virtual void setCurrentItem( const QModelIndex& index );

    virtual void setRepeatMode( PlaylistInterface::RepeatMode mode ) {}
    virtual void setShuffled( bool shuffled ) {}

protected:
    virtual void setReadOnly( bool b ) { m_readOnly = b; }

private:
    QPersistentModelIndex m_currentIndex;
    bool m_readOnly;
};

#endif // TRACKMODEL_H
