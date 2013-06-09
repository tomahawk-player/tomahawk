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

#define WEAKPEERHASH_KEY "WeakPeerHashKey"

WeakPeerHash::WeakPeerHash(QObject *parent)
    : QObject(parent)
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
WeakPeerHash::insert(const QString &key, const Tomahawk::peerinfo_ptr &value)
{
    value->setProperty( WEAKPEERHASH_KEY, key );
    connect( value.data(), SIGNAL( destroyed( QObject* ) ), SLOT( remove( QObject* ) ) );
    d_func()->hash.insert( key, value.toWeakRef() );
}

const QHash<QString, Tomahawk::peerinfo_wptr>&
WeakPeerHash::hash()
{
    return d_func()->hash;
}

void
WeakPeerHash::remove( QObject *value )
{
    const QString key = value->property( WEAKPEERHASH_KEY ).toString();
    d_func()->hash.remove( key );
}
