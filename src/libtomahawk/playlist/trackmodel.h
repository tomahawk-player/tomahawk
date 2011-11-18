/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011       Leo Franchi <lfranchi@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include <QAbstractItemModel>

#include "playlistinterface.h"
#include "trackmodelitem.h"
#include "typedefs.h"

#include "dllmacro.h"

class QMetaData;

class DLLEXPORT TrackModel : public QAbstractItemModel
{
Q_OBJECT

public:
    enum TrackItemStyle
    { Detailed = 0, Short = 1, ShortWithAvatars = 2 };

    enum TrackModelRole
    { StyleRole = Qt::UserRole + 1 };

    enum Columns {
        Artist = 0,
        Track = 1,
        Composer = 2,
        Album = 3,
        AlbumPos = 4,
        Duration = 5,
        Bitrate = 6,
        Age = 7,
        Year = 8,
        Filesize = 9,
        Origin = 10,
        Score = 11
    };

    explicit TrackModel( QObject* parent = 0 );
    virtual ~TrackModel();

    TrackModel::TrackItemStyle style() const { return m_style; }
    void setStyle( TrackModel::TrackItemStyle style );

    virtual QModelIndex index( int row, int column, const QModelIndex& parent ) const;
    virtual QModelIndex parent( const QModelIndex& child ) const;

    virtual bool isReadOnly() const { return m_readOnly; }
    virtual void setReadOnly( bool b ) { m_readOnly = b; }

    virtual QString title() const { return m_title; }
    virtual void setTitle( const QString& title ) { m_title = title; }
    virtual QString description() const { return m_description; }
    virtual void setDescription( const QString& description ) { m_description = description; }

    virtual int trackCount() const { return rowCount( QModelIndex() ); }

    virtual int rowCount( const QModelIndex& parent ) const;
    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    virtual QPersistentModelIndex currentItem() { return m_currentIndex; }
    virtual Tomahawk::QID currentItemUuid() { return m_currentUuid; }

    virtual Tomahawk::PlaylistInterface::RepeatMode repeatMode() const { return Tomahawk::PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void ensureResolved();

    TrackModelItem* itemFromIndex( const QModelIndex& index ) const;
    /// Returns a flat list of all tracks in this model
    QList< Tomahawk::query_ptr > queries() const;

signals:
    void repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );

    void loadingStarted();
    void loadingFinished();

public slots:
    virtual void setCurrentItem( const QModelIndex& index );

    virtual void clear();

    virtual void append( const QList< Tomahawk::query_ptr >& queries );
    virtual void append( const Tomahawk::query_ptr& query );
    virtual void append( const Tomahawk::artist_ptr& artist ) { Q_UNUSED( artist ); }
    virtual void append( const Tomahawk::album_ptr& album ) { Q_UNUSED( album ); }

    virtual void insert( const QList< Tomahawk::query_ptr >& queries, int row = 0 );
    virtual void insert( const Tomahawk::query_ptr& query, int row = 0 );

    virtual void remove( int row, bool moreToCome = false );
    virtual void remove( const QModelIndex& index, bool moreToCome = false );
    virtual void remove( const QList<QModelIndex>& indexes );
    virtual void remove( const QList<QPersistentModelIndex>& indexes );

    virtual void setRepeatMode( Tomahawk::PlaylistInterface::RepeatMode /*mode*/ ) {}
    virtual void setShuffled( bool /*shuffled*/ ) {}

protected:
    TrackModelItem* rootItem() const { return m_rootItem; }

private slots:
    void onPlaybackStarted( const Tomahawk::result_ptr& result );
    void onPlaybackStopped();

private:
    TrackModelItem* m_rootItem;
    QPersistentModelIndex m_currentIndex;
    Tomahawk::QID m_currentUuid;

    bool m_readOnly;

    QString m_title;
    QString m_description;

    TrackItemStyle m_style;
};

#endif // TRACKMODEL_H
