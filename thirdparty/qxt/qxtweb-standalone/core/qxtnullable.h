
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

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
