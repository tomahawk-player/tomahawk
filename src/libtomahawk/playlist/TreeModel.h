/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QPixmap>

#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "Result.h"
#include "PlaylistInterface.h"
#include "database/DatabaseCommand_AllArtists.h"

#include "TreeModelItem.h"
#include "infosystem/InfoSystem.h"

#include "DllMacro.h"
#include "Typedefs.h"

class QMetaData;

class DLLEXPORT TreeModel : public QAbstractItemModel
{
Q_OBJECT

public:
    enum Columns {
        Name = 0,
        Composer,
        Duration,
        Bitrate,
        Age,
        Year,
        Filesize,
        Origin,
        AlbumPosition
    };

    enum ColumnStyle
    { AllColumns = 0, TrackOnly };

    explicit TreeModel( QObject* parent = 0 );
    virtual ~TreeModel();

    virtual QModelIndex index( int row, int column, const QModelIndex& parent ) const;
    virtual QModelIndex parent( const QModelIndex& child ) const;

    virtual bool isReadOnly() const { return true; }

    virtual int trackCount() const { return rowCount( QModelIndex() ); }
    virtual int albumCount() const { return rowCount( QModelIndex() ); }

    virtual bool hasChildren( const QModelIndex& parent = QModelIndex() ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;

    virtual Tomahawk::ModelMode mode() const { return m_mode; }
    virtual void setMode( Tomahawk::ModelMode mode );

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual void clear();
    virtual void removeIndex( const QModelIndex& index );
    virtual void removeIndexes( const QList<QModelIndex>& indexes );

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    virtual QPersistentModelIndex currentItem() { return m_currentIndex; }

    Tomahawk::collection_ptr collection() const;

    void addAllCollections();
    void addCollection( const Tomahawk::collection_ptr& collection );
    void addFilteredCollection( const Tomahawk::collection_ptr& collection, unsigned int amount, DatabaseCommand_AllArtists::SortOrder order );

    void addArtists( const Tomahawk::artist_ptr& artist );
    void addTracks( const Tomahawk::album_ptr& album, const QModelIndex& parent, bool autoRefetch = false );
    void fetchAlbums( const Tomahawk::artist_ptr& artist );

    void getCover( const QModelIndex& index );

    ColumnStyle columnStyle() const { return m_columnStyle; }
    void setColumnStyle( ColumnStyle style );

    virtual QString title() const { return m_title; }
    virtual QString description() const { return m_description; }
    virtual QPixmap icon() const { return m_icon; }
    virtual void setTitle( const QString& title ) { m_title = title; }
    virtual void setDescription( const QString& description ) { m_description = description; }
    virtual void setIcon( const QPixmap& pixmap ) { m_icon = pixmap; }

    QModelIndex indexFromArtist( const Tomahawk::artist_ptr& artist ) const;
    TreeModelItem* itemFromIndex( const QModelIndex& index ) const
    {
        if ( index.isValid() )
        {
            return static_cast<TreeModelItem*>( index.internalPointer() );
        }
        else
        {
            return m_rootItem;
        }
    }

public slots:
    virtual void setCurrentItem( const QModelIndex& index );

    virtual void setRepeatMode( Tomahawk::PlaylistInterface::RepeatMode /*mode*/ ) {}
    virtual void setShuffled( bool /*shuffled*/ ) {}

    void addAlbums( const QModelIndex& parent, const QList<Tomahawk::album_ptr>& albums );

signals:
    void repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void modeChanged( Tomahawk::ModelMode mode );
    void itemCountChanged( unsigned int items );

    void loadingStarted();
    void loadingFinished();

protected:
    bool canFetchMore( const QModelIndex& parent ) const;
    void fetchMore( const QModelIndex& parent );

private slots:
    void onArtistsAdded( const QList<Tomahawk::artist_ptr>& artists );
    void onAlbumsFound( const QList<Tomahawk::album_ptr>& albums, Tomahawk::ModelMode mode );
    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const QModelIndex& index );
    void onTracksFound( const QList<Tomahawk::query_ptr>& tracks, const QVariant& variant );

    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );

    void onPlaybackStarted( const Tomahawk::result_ptr& result );
    void onPlaybackStopped();

    void onDataChanged();

    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onCollectionChanged();

private:
    QPersistentModelIndex m_currentIndex;
    TreeModelItem* m_rootItem;
    QString m_infoId;

    QString m_title;
    QString m_description;
    QPixmap m_icon;
    ColumnStyle m_columnStyle;
    Tomahawk::ModelMode m_mode;

    QList<Tomahawk::artist_ptr> m_artistsFilter;

    Tomahawk::collection_ptr m_collection;
    QList<Tomahawk::InfoSystem::InfoStringHash> m_receivedInfoData;
};

#endif // ALBUMMODEL_H
