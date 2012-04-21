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

#ifndef WIKIPEDIACONTEXT_H
#define WIKIPEDIACONTEXT_H

#include <QGraphicsProxyWidget>

#include "DllMacro.h"

#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "WebContext.h"

class DLLEXPORT WikipediaContext : public WebContext
{
Q_OBJECT

public:
    WikipediaContext() : WebContext() {}
    ~WikipediaContext() {}

    virtual Tomahawk::playlistinterface_ptr playlistInterface() const { return Tomahawk::playlistinterface_ptr(); }

    virtual QString title() const { return tr( "Wikipedia" ); }
    virtual QString description() const { return QString(); }

    virtual bool jumpToCurrentTrack() { return false; }

public slots:
    virtual void setArtist( const Tomahawk::artist_ptr& artist );
    virtual void setAlbum( const Tomahawk::album_ptr& album );
    virtual void setQuery( const Tomahawk::query_ptr& query );

private:
    Tomahawk::artist_ptr m_artist;
};


class DLLEXPORT LastfmContext : public WebContext
{
Q_OBJECT

public:
    LastfmContext() : WebContext() {}
    ~LastfmContext() {}

    virtual Tomahawk::playlistinterface_ptr playlistInterface() const { return Tomahawk::playlistinterface_ptr(); }

    virtual QString title() const { return tr( "Last.fm" ); }
    virtual QString description() const { return QString(); }

    virtual bool jumpToCurrentTrack() { return false; }

public slots:
    virtual void setArtist( const Tomahawk::artist_ptr& artist );
    virtual void setAlbum( const Tomahawk::album_ptr& album );
    virtual void setQuery( const Tomahawk::query_ptr& query );

private:
    Tomahawk::artist_ptr m_artist;
};

#endif // WIKIPEDIACONTEXT_H
