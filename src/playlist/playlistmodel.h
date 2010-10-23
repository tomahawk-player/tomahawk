#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QList>
#include <QHash>

#include "plitem.h"
#include "trackmodel.h"
#include "tomahawk/tomahawkapp.h"
#include "tomahawk/collection.h"
#include "tomahawk/query.h"
#include "tomahawk/typedefs.h"
#include "tomahawk/playlist.h"
#include "tomahawk/playlistinterface.h"

class QMetaData;

class PlaylistModel : public TrackModel
{
Q_OBJECT

public:
    explicit PlaylistModel( QObject* parent = 0 );
    ~PlaylistModel();

    int columnCount( const QModelIndex& parent = QModelIndex() ) const;

    QVariant data( const QModelIndex& index, int role ) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    void loadPlaylist( const Tomahawk::playlist_ptr& playlist );

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void itemSizeChanged( const QModelIndex& index );

    void loadingStarts();
    void loadingFinished();

private slots:
    void onDataChanged();

    void onRevisionLoaded( Tomahawk::PlaylistRevision revision );

private:
    Tomahawk::playlist_ptr m_playlist;
};

#endif // PLAYLISTMODEL_H
