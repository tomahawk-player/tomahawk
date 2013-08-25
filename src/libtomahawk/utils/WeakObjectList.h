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

#ifndef TOMAHAWK_UTILS_WEAKOBJECTLIST_H
#define TOMAHAWK_UTILS_WEAKOBJECTLIST_H


#include <QList>
#include <QObject>
#include <QWeakPointer>


namespace Tomahawk {
namespace Utils {

class WeakObjectListBase
{
public:
    virtual void remove( QObject* object );
    virtual ~WeakObjectListBase();
protected:
    WeakObjectListBase() {}
};

class WeakObjectListPrivate : public QObject
{
    Q_OBJECT
public:
    WeakObjectListPrivate( WeakObjectListBase* parent );

public slots:
    void remove( QObject* object );

private:
    WeakObjectListBase* m_parent;
};

template<class T>
class WeakObjectList : public WeakObjectListBase
{
    typedef QWeakPointer<T> wptr;
public:
    WeakObjectList() : m_private( this ) {}

    WeakObjectList( const WeakObjectList& list )
        : m_list( list.m_list )
        , m_private( this )
    {
    }

    void insert( const QSharedPointer<T>& value )
    {
        m_private.connect( value.data(), SIGNAL( destroyed( QObject* ) ), &m_private, SLOT( remove( QObject* )) );
        m_list.append( value.toWeakRef() );
    }

    const QList<wptr>& list() { return m_list; }
    QMutableListIterator< wptr > iter() { return QMutableListIterator< wptr >( m_list ); }
    virtual void remove( QObject* object )
    {
        QMutableListIterator< wptr > iter( m_list );
        while ( iter.hasNext() )
        {
            wptr ptr = iter.next();
            if ( ptr.data() == object || ptr.isNull() )
            {
                iter.remove();
            }
        }
    }

private:
    QList< wptr > m_list;
    WeakObjectListPrivate m_private;
};

} // namespace Utils
} // namespace Tomahawk

#endif // TOMAHAWK_UTILS_WEAKOBJECTLIST_H
