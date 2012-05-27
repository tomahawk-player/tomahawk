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

#ifndef TOPTRACKSCONTEXT_H
#define TOPTRACKSCONTEXT_H

#include <QGraphicsProxyWidget>

#include "DllMacro.h"

#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "context/ContextPage.h"

class PlaylistModel;
class PlaylistView;

class DLLEXPORT TopTracksContext : public Tomahawk::ContextPage
{
Q_OBJECT

public:
    TopTracksContext();
    ~TopTracksContext();

    virtual QGraphicsWidget* widget() { return m_proxy; }

    virtual Tomahawk::playlistinterface_ptr playlistInterface() const { return Tomahawk::playlistinterface_ptr(); }

    virtual QString title() const { return tr( "Top Hits" ); }
    virtual QString description() const { return QString(); }

    virtual bool jumpToCurrentTrack() { return false; }

public slots:
    virtual void setArtist( const Tomahawk::artist_ptr& artist );
    virtual void setAlbum( const Tomahawk::album_ptr& album );
    virtual void setQuery( const Tomahawk::query_ptr& query );

private slots:
    void onTracksFound( const QList<Tomahawk::query_ptr>& queries, Tomahawk::ModelMode mode );

private:
    PlaylistView* m_topHitsView;
    PlaylistModel* m_topHitsModel;

    QGraphicsProxyWidget* m_proxy;

    Tomahawk::artist_ptr m_artist;
};

#endif // TOPTRACKSCONTEXT_H
