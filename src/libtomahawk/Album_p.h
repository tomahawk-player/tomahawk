/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn  <uwelk@xhochy.com>
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

#pragma once
#ifndef ALBUM_P_H
#define ALBUM_P_H

#include "Album.h"

namespace Tomahawk
{

class AlbumPrivate
{
public:
    AlbumPrivate( Album* q, unsigned int _id, const QString& _name, const Tomahawk::artist_ptr& _artist )
        : q_ptr( q )
        , waitingForId( false )
        , id( _id )
        , name( _name )
        , artist( _artist )
        , coverLoaded( false )
        , coverLoading( false )
        , cover( 0 )
    {
    }


    AlbumPrivate( Album* q, const QString& _name, const Tomahawk::artist_ptr& _artist )
        : q_ptr( q )
        , waitingForId( true )
        , id( 0 )
        , name( _name )
        , artist( _artist )
        , coverLoaded( false )
        , coverLoading( false )
        , cover( 0 )
    {
    }

    Album* q_ptr;
    Q_DECLARE_PUBLIC( Album )

private:
    mutable bool waitingForId;
    mutable QFuture<unsigned int> idFuture;
    mutable unsigned int id;
    QString name;
    QString sortname;

    artist_ptr artist;

    mutable bool coverLoaded;
    mutable bool coverLoading;
    mutable QString uuid;

    mutable QByteArray coverBuffer;
    mutable QPixmap* cover;

    QHash< Tomahawk::ModelMode, QHash< Tomahawk::collection_ptr, Tomahawk::playlistinterface_ptr > > playlistInterface;

    QWeakPointer< Tomahawk::Album > ownRef;
};

} // namespace Tomahawk

#endif // ALBUM_P_H
