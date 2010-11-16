/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtCore module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/

/*!
\class QxtNullable QxtNullable
\inmodule QxtCore
\brief distinct null value compatible with any data type.

in general it's a templated abstraction to allow any data type to be
expressed with a null value distinct from any real value. An example
of such a use is for optional arguments.
\n
prepare a function for argument skipping:

\code
void  somefunction( qxtNull(int,a) , qxtNull(int,b) )
{

if (!a.isNull())
 {
 int i = a.value();
 //do something with i
 }
 if (!b.isNull())
 {
 int x = b.value();
 //do something with x
 }
}
\endcode

usage:
\code

somefunction(SKIP,1,2);
somefunction(3,4);
somefunction(3,SKIP,6);
somefunction(1);
\endcode

*/

#ifndef QXTNULLABLE_H
#define QXTNULLABLE_H
#include <qxtglobal.h>

/*! \relates QxtNullable
 * defines a skipable argument with type \a t and variable name \a n
 */
#define qxtNull(t,n)   QxtNullable<t> n = QxtNullable<t>()

#include <qxtnull.h>

template<typename T>
class /*QXT_CORE_EXPORT*/ QxtNullable
{
public:
    QxtNullable(QxtNull);
    QxtNullable(const T& p);
    QxtNullable();

    ///determinates if the Value is set to something meaningfull
    bool isNull() const;

    ///delete Value
    void nullify();

    T& value() const;
    operator T() const;
    void operator=(const T& p);

private:
    T* val;
};

template<typename T>
QxtNullable<T>::QxtNullable(QxtNull)
{
    val = 0;
}

template<typename T>
QxtNullable<T>::QxtNullable(const T& p)
{
    val = const_cast<T*>(&p);
}

template<typename T>
QxtNullable<T>::QxtNullable()
{
    val = 0;
}

template<typename T>
QxtNullable<T>::operator T() const
{
    return *val;
}

template<typename T>
T& QxtNullable<T>::value() const
{
    return *val;
}

template<typename T>
bool QxtNullable<T>::isNull() const
{
    return (val == 0);
}

template<typename T>
void QxtNullable<T>::nullify()
{
    val = 0;
}

template<typename T>
void QxtNullable<T>::operator=(const T & p)
{
    val = const_cast<T*>(&p);
}

#endif
