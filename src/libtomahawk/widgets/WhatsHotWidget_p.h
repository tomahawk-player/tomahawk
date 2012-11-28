/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef WHATSHOTWIDGET_P_H
#define WHATSHOTWIDGET_P_H

#include "WhatsHotWidget.h"
#include "PlaylistInterface.h"
#include "ui_WhatsHotWidget.h"
#include "playlist/TreeProxyModel.h"
#include "playlist/PlaylistView.h"
#include "Result.h"

#include <QObject>

namespace Tomahawk
{

class ChartsPlaylistInterface : public Tomahawk::PlaylistInterface
{
    Q_OBJECT
public:
    explicit ChartsPlaylistInterface( WhatsHotWidget* w )
        : PlaylistInterface()
        , m_w( w )
    {
        connect( m_w->ui->tracksViewLeft->proxyModel()->playlistInterface().data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ),
                 SLOT( anyRepeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );
        connect( m_w->ui->artistsViewLeft->proxyModel()->playlistInterface().data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ),
                 SLOT( anyRepeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );

        connect( m_w->ui->tracksViewLeft->proxyModel()->playlistInterface().data(), SIGNAL( shuffleModeChanged( bool ) ),
                 SLOT( anyShuffleChanged( bool ) ) );
        connect( m_w->ui->artistsViewLeft->proxyModel()->playlistInterface().data(), SIGNAL( shuffleModeChanged( bool ) ),
                 SLOT( anyShuffleChanged( bool ) ) );
    }
    virtual ~ChartsPlaylistInterface() {}

    // Any one is fine, we keep them all synched
    virtual PlaylistModes::RepeatMode repeatMode() const { return m_w->ui->tracksViewLeft->proxyModel()->playlistInterface()->repeatMode(); }
    virtual bool shuffled() const { return m_w->ui->tracksViewLeft->proxyModel()->playlistInterface()->shuffled(); }

    // Do nothing
    virtual void setCurrentIndex( qint64 index ) { Q_UNUSED( index ); }
    virtual Tomahawk::result_ptr resultAt( qint64 index ) const { Q_UNUSED( index ); Q_ASSERT( false ); return Tomahawk::result_ptr(); }
    virtual Tomahawk::query_ptr queryAt( qint64 index ) const { Q_UNUSED( index ); Q_ASSERT( false ); return Tomahawk::query_ptr(); }
    virtual qint64 indexOfResult( const Tomahawk::result_ptr& result ) const { Q_UNUSED( result ); Q_ASSERT( false ); return -1; }
    virtual qint64 indexOfQuery( const Tomahawk::query_ptr& query ) const { Q_UNUSED( query ); Q_ASSERT( false ); return -1; }
    virtual Tomahawk::result_ptr currentItem() const { return Tomahawk::result_ptr(); }
    virtual qint64 siblingIndex( int, qint64 rootIndex = -1 ) const { Q_UNUSED( rootIndex ); return -1; }
    virtual int trackCount() const { return 0; }
    virtual QList< Tomahawk::query_ptr > tracks() const { return QList< Tomahawk::query_ptr >(); }

    virtual bool hasChildInterface( Tomahawk::playlistinterface_ptr other )
    {
        return m_w->ui->tracksViewLeft->playlistInterface() == other ||
               m_w->ui->artistsViewLeft->playlistInterface() == other ||
               m_w->ui->albumsView->playlistInterface()->hasChildInterface( other );

    }

public slots:
    virtual void setRepeatMode( PlaylistModes::RepeatMode mode )
    {
        m_w->ui->tracksViewLeft->proxyModel()->playlistInterface()->setRepeatMode( mode );
        m_w->ui->artistsViewLeft->proxyModel()->playlistInterface()->setRepeatMode( mode );
    }

    virtual void setShuffled( bool enabled )
    {
        m_w->ui->tracksViewLeft->proxyModel()->playlistInterface()->setShuffled( enabled );
        m_w->ui->artistsViewLeft->proxyModel()->playlistInterface()->setShuffled( enabled );
    }

private slots:
    void anyRepeatModeChanged( Tomahawk::PlaylistModes::RepeatMode mode )
    {
        emit repeatModeChanged( mode );
    }

    void anyShuffleChanged( bool enabled )
    {
        emit shuffleModeChanged( enabled );
    }

private:
    WhatsHotWidget* m_w;

};

} //ns

#endif
