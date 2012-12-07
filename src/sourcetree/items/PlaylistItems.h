/*
 *    Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef PLAYLIST_ITEM_H
#define PLAYLIST_ITEM_H

#include "SourceTreeItem.h"
#include "playlist/dynamic/DynamicPlaylist.h"

class PlaylistItem : public SourceTreeItem
{
    Q_OBJECT
public:
    PlaylistItem( SourcesModel* model, SourceTreeItem* parent, const Tomahawk::playlist_ptr& pl, int index = -1 );

    virtual QString text() const;
    virtual Tomahawk::playlist_ptr playlist() const;
    virtual Qt::ItemFlags flags() const;
    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual DropTypes supportedDropTypes( const QMimeData* data ) const;
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action );
    virtual QIcon icon() const;
    virtual bool setData(const QVariant& v, bool role);
    virtual int peerSortValue() const;
    virtual int IDValue() const;
    virtual bool isBeingPlayed() const;

    virtual SourceTreeItem* activateCurrent();

    // subscription management
    bool canSubscribe() const;
    bool subscribed() const;
    QPixmap subscribedIcon() const;
    void setSubscribed( bool subscribed );
    bool collaborative() const;

public slots:
    virtual void activate();
    virtual void doubleClicked();

protected:
    void setLoaded( bool loaded );

private slots:
    void onPlaylistLoaded( Tomahawk::PlaylistRevision revision );
    void onPlaylistChanged();
    void parsedDroppedTracks( const QList<Tomahawk::query_ptr>& tracks );

    void onUpdated();
private:
    bool createOverlay();

    bool m_loaded, m_canSubscribe, m_showSubscribed;
    Tomahawk::playlist_ptr m_playlist;
    QIcon m_overlaidIcon;
    QPixmap m_subscribedOnIcon, m_subscribedOffIcon;
    QList<Tomahawk::PlaylistUpdaterInterface*> m_overlaidUpdaters;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(PlaylistItem::DropTypes)

// can be a station or an auto playlist
class DynamicPlaylistItem : public PlaylistItem
{
    Q_OBJECT
public:
    DynamicPlaylistItem( SourcesModel* model, SourceTreeItem* parent, const Tomahawk::dynplaylist_ptr& pl, int index = -1 );
    virtual ~DynamicPlaylistItem();

    virtual QString text() const;
    Tomahawk::dynplaylist_ptr dynPlaylist() const;
    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual void activate();
    virtual int peerSortValue() const;
    virtual int IDValue() const;
    virtual QIcon icon() const;

    virtual SourceTreeItem* activateCurrent();
    virtual bool isBeingPlayed() const;

private slots:
    void onDynamicPlaylistLoaded( Tomahawk::DynamicPlaylistRevision revision );

private:
    void checkReparentHackNeeded( const Tomahawk::DynamicPlaylistRevision& rev );

    Tomahawk::dynplaylist_ptr m_dynplaylist;
};


#endif
