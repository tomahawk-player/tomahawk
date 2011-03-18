#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include <QAbstractItemModel>

#include "playlistinterface.h"
#include "playlist/plitem.h"

#include "dllmacro.h"

class QMetaData;

class DLLEXPORT TrackModel : public QAbstractItemModel
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
        Year = 6,
        Filesize = 7,
        Origin = 8
    };

    explicit TrackModel( QObject* parent = 0 );
    virtual ~TrackModel();

    virtual QModelIndex index( int row, int column, const QModelIndex& parent ) const;
    virtual QModelIndex parent( const QModelIndex& child ) const;

    virtual bool isReadOnly() const { return m_readOnly; }

    virtual QString title() const { return m_title; }
    virtual void setTitle( const QString& title ) { m_title = title; }
    virtual QString description() const { return m_description; }
    virtual void setDescription( const QString& description ) { m_description = description; }

    virtual int trackCount() const { return rowCount( QModelIndex() ); }

    virtual int rowCount( const QModelIndex& parent ) const;
    virtual int columnCount( const QModelIndex& parent ) const;

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    virtual QPersistentModelIndex currentItem() { return m_currentIndex; }

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void append( const Tomahawk::query_ptr& query ) = 0;

    PlItem* itemFromIndex( const QModelIndex& index ) const;

    PlItem* m_rootItem;

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );

    void loadingStarted();
    void loadingFinished();

public slots:
    virtual void setCurrentItem( const QModelIndex& index );

    virtual void removeIndex( const QModelIndex& index, bool moreToCome = false );
    virtual void removeIndexes( const QList<QModelIndex>& indexes );

    virtual void setRepeatMode( PlaylistInterface::RepeatMode mode ) {}
    virtual void setShuffled( bool shuffled ) {}

protected:
    virtual void setReadOnly( bool b ) { m_readOnly = b; }

private slots:
    void onPlaybackFinished( const Tomahawk::result_ptr& result );
    void onPlaybackStopped();

private:
    QPersistentModelIndex m_currentIndex;
    bool m_readOnly;

    QString m_title;
    QString m_description;
};

#endif // TRACKMODEL_H
