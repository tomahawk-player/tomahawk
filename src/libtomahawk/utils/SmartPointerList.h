/****************************************************************************************
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2009 Max Howell <max@last.fm>                                          *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef SMART_POINTER_LIST_H
#define SMART_POINTER_LIST_H

#include <QList>   //baseclass
#include <QObject> //baseclass

#include "DllMacro.h"

class DLLEXPORT SmartPointerListDaddy : public QObject
{
    Q_OBJECT
    QList<QObject*>& m_list;

public:
    SmartPointerListDaddy( QList<QObject*>* list ) : m_list( *list )
    {}

private slots:
    void onDestroyed()
    {
        m_list.removeAll( sender() );
    }
};

/** QList has no virtual functions, so we inherit privately and define the
  * interface exactly to ensure users can't write code that breaks the
  * class's internal behaviour.
  *
  * I deliberately didn't define clear. I worried people would assume it
  * deleted the pointers. Or assume it didn't. I didn't expose a few other
  * functions for that reason.
  *
  * non-const iterator functions are not exposed as they access the QList
  * baseclass, and then the Daddy wouldn't be watching newly inserted items.
  *
  * --mxcl
  * Exposed clear. This class doesn't have a QPtrList autodelete functionality
  * ever, so if people think that, they're really confused! -- Ian Monroe
  *
  * NOTE:
  *       This class is NOT implicitly shared like QList. Passing it around
  *       ***will*** cause it to iterate and copy all the elements in the copy
  *       constructor!
  *
  */
template <class T> class SmartPointerList : private QList<T*>
{
    class SmartPointerListDaddy* m_daddy;

public:
    SmartPointerList() : m_daddy( new SmartPointerListDaddy( (QList<QObject*>*)this ) )
    {}

    ~SmartPointerList()
    {
        delete m_daddy;
    }

    SmartPointerList( const SmartPointerList<T>& that )
            : QList<T*>()
            , m_daddy( new SmartPointerListDaddy( (QList<QObject*>*)this ) )
    {
        QListIterator<T*> i( that );
        while (i.hasNext())
            append( i.next() );
    }

    SmartPointerList& operator=( const SmartPointerList<T>& that )
    {
        QListIterator<T*> i( *this);
        while (i.hasNext())
            QObject::disconnect( m_daddy, 0, i.next(), 0 );

        QList<T*>::operator=( that );

        if (this != &that) {
            QListIterator<T*> i( that );
            while (i.hasNext())
                m_daddy->connect( i.next(), SIGNAL(destroyed()), SLOT(onDestroyed()) );
        }

        return *this;
    }

    // keep same function names as Qt
    void append( T* o )
    {
        m_daddy->connect( o, SIGNAL(destroyed()), SLOT(onDestroyed()) );
        QList<T*>::append( o );
    }

    void prepend( T* o )
    {
        m_daddy->connect( o, SIGNAL(destroyed()), SLOT(onDestroyed()) );
        QList<T*>::prepend( o );
    }

    SmartPointerList& operator+=( T* o )
    {
        append( o );
        return *this;
    }

    SmartPointerList& operator<<( T* o )
    {
        return operator+=( o );
    }

    SmartPointerList operator+( const SmartPointerList that )
    {
        SmartPointerList<T> copy = *this;
        QListIterator<T*> i( that );
        while (i.hasNext())
            copy.append( i.next() );
        return copy;
    }

    SmartPointerList& operator+=( const SmartPointerList that )
    {
        QListIterator<T*> i( that );
        while (i.hasNext())
            append( i.next() );
        return *this;
    }

    bool operator==( const SmartPointerList& that )
    {
        return QList<T*>::operator==( that );
    }

    void push_back( T* o )
    {
        append( o );
    }

    void push_front( T* o )
    {
        prepend( o );
    }

    void replace( int i, T* o )
    {
        QList<T*>::replace( i, o );
        m_daddy->connect( o, SIGNAL(destroyed()), SLOT(onDestroyed()) );
    }

    /** this is a "safe" class. We always bounds check */
    inline T* operator[]( int index ) const { return QList<T*>::value( index ); }
    inline T* at( int index ) const { return QList<T*>::value( index ); }

    // make public safe functions again
    using QList<T*>::back;
    using QList<T*>::constBegin;
    using QList<T*>::constEnd;
    using typename QList<T*>::const_iterator;
    using QList<T*>::contains;
    using QList<T*>::count;
    using QList<T*>::empty;
    using QList<T*>::erase;
    using QList<T*>::first;
    using QList<T*>::front;
    using QList<T*>::indexOf;
    using QList<T*>::insert;
    using QList<T*>::isEmpty;
    using QList<T*>::last;
    using QList<T*>::lastIndexOf;
    using QList<T*>::mid;
    using QList<T*>::move;
    using QList<T*>::pop_back;
    using QList<T*>::pop_front;
    using QList<T*>::size;
    using QList<T*>::swap;
    using QList<T*>::value;
    using QList<T*>::operator!=;
//     using QList<T*>::operator==;

    // can't use using directive here since we only want the const versions
    typename QList<T*>::const_iterator begin() const { return QList<T*>::constBegin(); }
    typename QList<T*>::const_iterator end() const { return QList<T*>::constEnd(); }

    // it can lead to poor performance situations if we don't disconnect
    // but I think it's not worth making this class more complicated for such
    // an edge case
    using QList<T*>::clear;
    using QList<T*>::removeAll;
    using QList<T*>::removeAt;
    using QList<T*>::removeFirst;
    using QList<T*>::removeLast;
    using QList<T*>::removeOne;
    using QList<T*>::takeAt;
    using QList<T*>::takeFirst;
    using QList<T*>::takeLast;
};

#endif //HEADER_GUARD

