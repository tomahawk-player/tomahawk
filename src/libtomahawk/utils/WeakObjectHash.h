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
#include "utils/Closure.h"

#include <QObject>

namespace Tomahawk
{

namespace Utils
{

class WeakObjectHashBase
{
public:
    virtual void remove( const QString& key );
    virtual ~WeakObjectHashBase();
protected:
    WeakObjectHashBase() {}
};

class WeakObjectHashPrivate : public QObject
{
    Q_OBJECT
public:
    WeakObjectHashPrivate( WeakObjectHashBase* parent );

public slots:
    void remove( const QString& key );

private:
    WeakObjectHashBase* m_parent;
};

template<class T>
class WeakObjectHash : public WeakObjectHashBase
{
public:
    WeakObjectHash() : m_private( this ) {}

    WeakObjectHash( const WeakObjectHash& hash )
        : m_hash( hash.m_hash )
        , m_private( this )
    {
    }

    void insert( const QString& key, const QSharedPointer<T>& value )
    {
        // Do not pass the QSharedPointer to the closure as this will prevent the object from being destroyed.
        _detail::Closure* cl = NewClosure( value.data(), SIGNAL( destroyed( QObject* ) ), &m_private, SLOT( remove( QString ) ), key );
        cl->setAutoDelete( true );
        m_hash.insert( key, value.toWeakRef() );
    }

    const QHash< QString, QWeakPointer<T> >& hash() { return m_hash; }
    virtual void remove( const QString& key ) { m_hash.remove( key ); }

private:
    QHash< QString, QWeakPointer<T> > m_hash;
    WeakObjectHashPrivate m_private;
};

} // namespace Utils

} // namespace Tomahawk

#endif // WEAKPEERHASH_H
