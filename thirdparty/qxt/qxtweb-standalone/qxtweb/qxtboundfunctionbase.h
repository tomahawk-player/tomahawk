
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
