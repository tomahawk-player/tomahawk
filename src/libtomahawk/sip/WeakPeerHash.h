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

#ifndef WEAKPEERHASH_H
#define WEAKPEERHASH_H

#include "Typedefs.h"

#include <QObject>

class WeakPeerHashPrivate;

class WeakPeerHash : public QObject
{
    Q_OBJECT
public:
    WeakPeerHash( QObject *parent = 0 );
    WeakPeerHash( const WeakPeerHash& hash );
    void insert( const QString& key, const Tomahawk::peerinfo_ptr& value );
    const QHash< QString, Tomahawk::peerinfo_wptr>& hash();

signals:
    
private slots:
    void remove( QObject* value );
private:
    Q_DECLARE_PRIVATE( WeakPeerHash )
    WeakPeerHashPrivate* d_ptr;

};

#endif // WEAKPEERHASH_H
