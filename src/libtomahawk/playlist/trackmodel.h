/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
        Album = 2,
        AlbumPos = 3,
        Duration = 4,
        Bitrate = 5,
        Age = 6,
        Year = 7,
        Filesize = 8,
        Origin = 9,
        Score = 10,
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

    virtual Tomahawk::PlaylistInterface::RepeatMode repeatMode() const { return Tomahawk::PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void ensureResolved();

    virtual void append( const Tomahawk::query_ptr& query ) = 0;
    virtual void append( const Tomahawk::artist_ptr& artist ) = 0;
    virtual void append( const Tomahawk::album_ptr& album ) = 0;

    TrackModelItem* itemFromIndex( const QModelIndex& index ) const;
    TrackModelItem* m_rootItem;

signals:
    void repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );

    void loadingStarted();
    void loadingFinished();

public slots:
    virtual void setCurrentItem( const QModelIndex& index );

    virtual void removeIndex( const QModelIndex& index, bool moreToCome = false );
    virtual void removeIndexes( const QList<QModelIndex>& indexes );

    virtual void setRepeatMode( Tomahawk::PlaylistInterface::RepeatMode /*mode*/ ) {}
    virtual void setShuffled( bool /*shuffled*/ ) {}

private slots:
    void onPlaybackFinished( const Tomahawk::result_ptr& result );
    void onPlaybackStopped();

private:
    QPersistentModelIndex m_currentIndex;
    bool m_readOnly;

    QString m_title;
    QString m_description;

    TrackItemStyle m_style;
};

#endif // TRACKMODEL_H
