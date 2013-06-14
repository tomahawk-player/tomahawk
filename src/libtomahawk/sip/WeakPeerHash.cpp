/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "WeakPeerHash_p.h"

#include "PeerInfo.h"
#include "utils/Closure.h"

#define WEAKPEERHASH_KEY "WeakPeerHashKey"

WeakPeerHash::WeakPeerHash( QObject *parent )
    : QObject( parent )
    , d_ptr( new WeakPeerHashPrivate( this ) )
{
}

WeakPeerHash::WeakPeerHash( const WeakPeerHash &hash )
    : QObject( hash.parent() )
    , d_ptr( new WeakPeerHashPrivate( this ) )
{
    d_func()->hash = hash.d_func()->hash;
}

void
WeakPeerHash::insert( const QString &key, const Tomahawk::peerinfo_ptr &value )
{
    _detail::Closure* cl = NewClosure( value, SIGNAL( destroyed( QObject* ) ), this, SLOT( remove( QString ) ), key );
    cl->setAutoDelete( true );
    d_func()->hash.insert( key, value.toWeakRef() );
}

const QHash<QString, Tomahawk::peerinfo_wptr>&
WeakPeerHash::hash()
{
    return d_func()->hash;
}

void
WeakPeerHash::remove( const QString& key )
{
    d_func()->hash.remove( key );
}
