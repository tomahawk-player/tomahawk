/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
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

#ifndef ALBUMINFOWIDGET_P_H
#define ALBUMINFOWIDGET_P_H

#include "AlbumInfoWidget.h"
#include "ui_AlbumInfoWidget.h"
#include "PlaylistInterface.h"
#include "TreeProxyModel.h"
#include "Result.h"
#include "Typedefs.h"

#include <QObject>

class MetaAlbumInfoInterface : public Tomahawk::PlaylistInterface
{
    Q_OBJECT
public:
    explicit MetaAlbumInfoInterface( AlbumInfoWidget* w )
        : PlaylistInterface()
        , m_w( w )
    {
        connect( m_w->ui->tracksView->proxyModel()->playlistInterface().data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ),
                 SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );

        connect( m_w->ui->tracksView->proxyModel()->playlistInterface().data(), SIGNAL( shuffleModeChanged( bool ) ),
                 SIGNAL( shuffleModeChanged( bool ) ) );
    }
    virtual ~MetaAlbumInfoInterface() {}


    virtual Tomahawk::PlaylistModes::RepeatMode repeatMode() const { return m_w->ui->tracksView->proxyModel()->playlistInterface()->repeatMode(); }
    virtual bool shuffled() const { return m_w->ui->tracksView->proxyModel()->playlistInterface()->shuffled(); }

    virtual Tomahawk::result_ptr currentItem() const { return m_w->ui->tracksView->proxyModel()->playlistInterface()->currentItem(); }
    virtual Tomahawk::result_ptr siblingItem( int itemsAway ) { return m_w->ui->tracksView->proxyModel()->playlistInterface()->siblingItem( itemsAway ); }
    virtual int trackCount() const { return m_w->ui->tracksView->proxyModel()->playlistInterface()->trackCount(); }
    virtual QList< Tomahawk::query_ptr > tracks() { return m_w->ui->tracksView->proxyModel()->playlistInterface()->tracks(); }

    virtual bool hasChildInterface( Tomahawk::playlistinterface_ptr other )
    {
        return m_w->ui->tracksView->proxyModel()->playlistInterface() == other ||
               m_w->ui->tracksView->proxyModel()->playlistInterface()->hasChildInterface( other ) ||
               m_w->ui->albumsView->playlistInterface()->hasChildInterface( other );
    }

    virtual void setRepeatMode( Tomahawk::PlaylistModes::RepeatMode mode )
    {
        m_w->ui->tracksView->proxyModel()->playlistInterface()->setRepeatMode( mode );
    }

    virtual void setShuffled( bool enabled )
    {
        m_w->ui->tracksView->proxyModel()->playlistInterface()->setShuffled( enabled );
    }

private:
    AlbumInfoWidget* m_w;

};

#endif
