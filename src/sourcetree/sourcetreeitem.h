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

#ifndef SOURCETREEITEM_H
#define SOURCETREEITEM_H

#include <QObject>
#include <QStandardItem>

#include "typedefs.h"
#include "sourcetreeitemwidget.h"
#include "playlist/dynamic/DynamicPlaylist.h"

class SourceTreeItem : public QObject
{
Q_OBJECT
public:
    
    enum PlaylistItemType {
        Type = Qt::UserRole + 1, /// Value is SourcesModel::SourceType
        SourceItemPointer = Qt::UserRole + 2, /// value is the sourcetreeritem of the collection itself.
        PlaylistPointer = Qt::UserRole + 3,  /// Value is the playlist_ptr.data()
        DynamicPlaylistPointer = Qt::UserRole + 4 /// Value is the dynplaylist_ptr.data()
    };

    explicit SourceTreeItem( const Tomahawk::source_ptr& source, QObject* parent );
    virtual ~SourceTreeItem();

    const Tomahawk::source_ptr& source() const { return m_source; };
    QList<QStandardItem*> columns() const { return m_columns; };

    QWidget* widget() const { return m_widget; };

    // returns revision ID we are curently displaying for given playlist ID
    QString currentlyLoadedPlaylistRevision( const QString& plguid ) const
    {
        return m_current_revisions.value( plguid );
    }

signals:
    void clicked( const QModelIndex& index );

public slots:
    void onOnline();
    void onOffline();

private slots:
    void onClicked();

    void onPlaylistsAdded( const QList<Tomahawk::playlist_ptr>& playlists );
    void onPlaylistsDeleted( const QList<Tomahawk::playlist_ptr>& playlists );
    void onPlaylistLoaded( Tomahawk::PlaylistRevision revision );
    void onPlaylistChanged();

    void onDynamicPlaylistsAdded( const QList<Tomahawk::dynplaylist_ptr>& playlists );
    void onDynamicPlaylistsDeleted( const QList<Tomahawk::dynplaylist_ptr>& playlists );
    void onDynamicPlaylistLoaded( Tomahawk::DynamicPlaylistRevision revision );

private:
    void playlistAddedInternal( qlonglong ptr, const Tomahawk::playlist_ptr& pl, bool dynamic );

    QList<QStandardItem*> m_columns;
    Tomahawk::source_ptr m_source;
    SourceTreeItemWidget* m_widget;
    QList<Tomahawk::playlist_ptr> m_playlists;
    QList<Tomahawk::dynplaylist_ptr> m_dynplaylists;

    // playist->guid() -> currently loaded revision
    QMap<QString,QString> m_current_revisions;
    QMap<QString,QString> m_current_dynamic_revisions;
};

#endif // SOURCETREEITEM_H
