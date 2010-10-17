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

#ifndef QXTNULL_H
#define QXTNULL_H

#include <qxtglobal.h>

/*!
\class QxtNull QxtNull

\inmodule QxtCore

\brief An object representing the "null" value for QxtNullable.

\sa QxtNullable
*/

struct QXT_CORE_EXPORT QxtNull
{
    /*! integer cast operator
     * In expressions, QxtNull behaves as an integer zero for compatibility with generic functions.
     */
    operator int() const
    {
        return 0;
    }
    enum { isNull = true };
};

#ifndef QXT_NO_MACROS

/*! \relates QxtNull
 * A convenience alias for QxtNull().
 */
#define QXT_NULL QxtNull()

#endif // QXT_NO_MACROS

#endif // QXTNULL_H
