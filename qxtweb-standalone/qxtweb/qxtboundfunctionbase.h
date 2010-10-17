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

// This file exists for the convenience of QxtBoundCFunction.
// It is not part of the public API and is subject to change.
//
// We mean it.

#ifndef QXTBOUNDFUNCTIONBASE_H
#define QXTBOUNDFUNCTIONBASE_H

#include <QObject>
#include <QMetaObject>
#include <QGenericArgument>
#include <qxtmetaobject.h>
#include <qxtboundfunction.h>

#ifndef QXT_DOXYGEN_RUN

#define QXT_10_UNUSED Q_UNUSED(p1) Q_UNUSED(p2) Q_UNUSED(p3) Q_UNUSED(p4) Q_UNUSED(p5) Q_UNUSED(p6) Q_UNUSED(p7) Q_UNUSED(p8) Q_UNUSED(p9) Q_UNUSED(p10)

class QXT_CORE_EXPORT QxtBoundFunctionBase : public QxtBoundFunction
{
public:
    QByteArray bindTypes[10];
    QGenericArgument arg[10], p[10];
    void* data[10];

    QxtBoundFunctionBase(QObject* parent, QGenericArgument* params[10], QByteArray types[10]);
    virtual ~QxtBoundFunctionBase();

    int qt_metacall(QMetaObject::Call _c, int _id, void **_a);
    bool invokeBase(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QGenericArgument));
};

#define QXT_ARG(i) ((argCount>i)?QGenericArgument(p ## i .typeName(), p ## i .constData()):QGenericArgument())
#define QXT_VAR_ARG(i) (p ## i .isValid())?QGenericArgument(p ## i .typeName(), p ## i .constData()):QGenericArgument()
#endif
#endif
