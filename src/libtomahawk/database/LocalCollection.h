/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef LOCALCOLLECTION_H
#define LOCALCOLLECTION_H

#include "DatabaseCollection.h"
#include "DllMacro.h"
#include "Playlist.h"

class DLLEXPORT LocalCollection : public DatabaseCollection
{
    Q_OBJECT
public:
    explicit LocalCollection( const Tomahawk::source_ptr& source, QObject* parent = 0 );

    virtual QString prettyName() const;
    virtual QString emptyText() const;

    // gets the playlist used for storing stuff from the web, if it already exists. if the returned playlist
    // is invalid ask to create and listen to the signal
    Tomahawk::playlist_ptr bookmarksPlaylist();
    void createBookmarksPlaylist();

signals:
    void bookmarkPlaylistCreated( const Tomahawk::playlist_ptr& p );

private slots:
    void created();

};

#endif // LOCALCOLLECTION_H
