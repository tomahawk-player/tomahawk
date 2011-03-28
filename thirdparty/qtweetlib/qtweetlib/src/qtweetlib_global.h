/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#ifndef QTWEETLIB_GLOBAL_H
#define QTWEETLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#ifndef QTWEETLIBSHARED_EXPORT 
# if defined (QTWEETLIB_MAKEDLL)
#  define QTWEETLIBSHARED_EXPORT Q_DECL_EXPORT
# else
#  define QTWEETLIBSHARED_EXPORT Q_DECL_IMPORT
# endif
#endif

#define AUTH_HEADER "Authorization"

#endif // QTWEETLIB_GLOBAL_H
