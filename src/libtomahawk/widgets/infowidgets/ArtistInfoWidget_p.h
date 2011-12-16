/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef ARTISTINFOWIDGET_P_H
#define ARTISTINFOWIDGET_P_H

#include "ArtistInfoWidget.h"
#include "ui_ArtistInfoWidget.h"
#include "playlistinterface.h"
#include "treeproxymodel.h"
#include "result.h"

#include <QObject>

class MetaPlaylistInterface : public QObject, public Tomahawk::PlaylistInterface
{
    Q_OBJECT
public:
    explicit MetaPlaylistInterface( ArtistInfoWidget* w )
        : PlaylistInterface( this )
        , m_w( w )
    {
        connect( m_w->ui->albums->proxyModel(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ),
                 SLOT( anyRepeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ) );
        connect( m_w->ui->relatedArtists->proxyModel(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ),
                 SLOT( anyRepeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ) );
        connect( m_w->ui->topHits->proxyModel(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ),
                 SLOT( anyRepeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ) );

        connect( m_w->ui->albums->proxyModel(), SIGNAL( shuffleModeChanged( bool ) ),
                 SLOT( anyShuffleChanged( bool ) ) );
        connect( m_w->ui->relatedArtists->proxyModel(), SIGNAL( shuffleModeChanged( bool ) ),
                 SLOT( anyShuffleChanged( bool ) ) );
        connect( m_w->ui->topHits->proxyModel(), SIGNAL( shuffleModeChanged( bool ) ),
                 SLOT( anyShuffleChanged( bool ) ) );
    }
    virtual ~MetaPlaylistInterface() {}


    // Any one is fine, we keep them all synched
    virtual RepeatMode repeatMode() const { return m_w->ui->albums->proxyModel()->repeatMode(); }

    virtual bool shuffled() const { return m_w->ui->albums->proxyModel()->shuffled(); }

    // Do nothing
    virtual Tomahawk::result_ptr currentItem() const { return Tomahawk::result_ptr(); }
    virtual Tomahawk::result_ptr siblingItem( int ) { return Tomahawk::result_ptr(); }
    virtual int trackCount() const { return 0; }
    virtual QList< Tomahawk::query_ptr > tracks() { return QList< Tomahawk::query_ptr >(); }
    virtual int unfilteredTrackCount() const { return 0; }

    virtual bool hasChildInterface( PlaylistInterface* other )
    {
        return ( m_w->ui->albums->playlistInterface() == other ) ||
               ( m_w->ui->relatedArtists->playlistInterface() == other ) ||
               ( m_w->ui->topHits->playlistInterface() == other );
    }
public slots:
    virtual void setRepeatMode( RepeatMode mode )
    {
        m_w->ui->albums->proxyModel()->setRepeatMode( mode );
        m_w->ui->relatedArtists->proxyModel()->setRepeatMode( mode );
        m_w->ui->topHits->proxyModel()->setRepeatMode( mode );
    }

    virtual void setShuffled( bool enabled )
    {
        m_w->ui->albums->proxyModel()->setShuffled( enabled );
        m_w->ui->relatedArtists->proxyModel()->setShuffled( enabled );
        m_w->ui->topHits->proxyModel()->setShuffled( enabled );
    }

signals:
    void repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );
    void nextTrackReady();

private slots:
    void anyRepeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode mode )
    {
        emit repeatModeChanged( mode );
    }

    void anyShuffleChanged( bool enabled )
    {
        emit shuffleModeChanged( enabled );
    }

private:
    ArtistInfoWidget* m_w;

};

#endif
