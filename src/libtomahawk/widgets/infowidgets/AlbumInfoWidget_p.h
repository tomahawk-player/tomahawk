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
#include "playlist/TreeProxyModel.h"
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
        connect( m_w->ui->tracks->proxyModel()->playlistInterface().data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ),
                 SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );

        connect( m_w->ui->tracks->proxyModel()->playlistInterface().data(), SIGNAL( shuffleModeChanged( bool ) ),
                 SIGNAL( shuffleModeChanged( bool ) ) );
    }
    virtual ~MetaAlbumInfoInterface() {}

    virtual Tomahawk::PlaylistModes::RepeatMode repeatMode() const { return m_w->ui->tracks->proxyModel()->playlistInterface()->repeatMode(); }
    virtual bool shuffled() const { return m_w->ui->tracks->proxyModel()->playlistInterface()->shuffled(); }

    virtual void setCurrentIndex( qint64 index ) { Q_UNUSED( index ); }
    virtual Tomahawk::result_ptr resultAt( qint64 index ) const { Q_UNUSED( index ); Q_ASSERT( false ); return Tomahawk::result_ptr(); }
    virtual Tomahawk::query_ptr queryAt( qint64 index ) const { Q_UNUSED( index ); Q_ASSERT( false ); return Tomahawk::query_ptr(); }
    virtual qint64 indexOfResult( const Tomahawk::result_ptr& result ) const { Q_UNUSED( result ); Q_ASSERT( false ); return -1; }
    virtual qint64 indexOfQuery( const Tomahawk::query_ptr& query ) const { Q_UNUSED( query ); Q_ASSERT( false ); return -1; }
    virtual Tomahawk::result_ptr currentItem() const { return m_w->ui->tracks->proxyModel()->playlistInterface()->currentItem(); }
    virtual qint64 siblingIndex( int itemsAway, qint64 rootIndex = -1 ) const { return m_w->ui->tracks->proxyModel()->playlistInterface()->siblingIndex( itemsAway, rootIndex ); }
    virtual int trackCount() const { return m_w->ui->tracks->proxyModel()->playlistInterface()->trackCount(); }
    virtual QList< Tomahawk::query_ptr > tracks() const { return m_w->ui->tracks->proxyModel()->playlistInterface()->tracks(); }

    virtual bool hasChildInterface( Tomahawk::playlistinterface_ptr other )
    {
        return m_w->ui->tracks->proxyModel()->playlistInterface() == other ||
               m_w->ui->tracks->proxyModel()->playlistInterface()->hasChildInterface( other ) ||
               m_w->ui->albums->playlistInterface()->hasChildInterface( other );
    }

    virtual void setRepeatMode( Tomahawk::PlaylistModes::RepeatMode mode )
    {
        m_w->ui->tracks->proxyModel()->playlistInterface()->setRepeatMode( mode );
    }

    virtual void setShuffled( bool enabled )
    {
        m_w->ui->tracks->proxyModel()->playlistInterface()->setShuffled( enabled );
    }

private:
    AlbumInfoWidget* m_w;

};

#endif
