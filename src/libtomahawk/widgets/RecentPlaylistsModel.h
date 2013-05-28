/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef RECENTLPLAYLISTSMODEL_H
#define RECENTLPLAYLISTSMODEL_H

// #include "Playlist.h"
// #include "Source.h"
#include "database/DatabaseCommand_LoadAllSortedPlaylists.h" // private slot

#include "dynplaylist_ptr.h"

#include <QModelIndex>

class QTimer;

class RecentPlaylistsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RecentPlaylistsModel( unsigned int maxPlaylists, QObject* parent = 0 );

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;

public slots:
    void refresh();
    void onReady();

signals:
    void emptinessChanged( bool isEmpty );

    void loadingStarted();
    void loadingFinished();

private slots:
    void onRefresh();
    void playlistsLoaded( const QList<DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair>& playlistGuids );

    void onPlaylistsRemoved( QList< Tomahawk::playlist_ptr > playlists );
    void onDynPlaylistsRemoved( QList< Tomahawk::dynplaylist_ptr > playlists );
    void updatePlaylist();

    void sourceOnline();
    void onSourceAdded( const Tomahawk::source_ptr& source );

private:
    QList< Tomahawk::playlist_ptr > m_playlists;
    mutable QHash< Tomahawk::playlist_ptr, QString > m_artists;
    unsigned int m_maxPlaylists;
    QTimer* m_timer;
};

#endif // RECENTLPLAYLISTSMODEL_H
