#ifndef QXTMETAOBJECT_H
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

#define QXTMETAOBJECT_H

#include <QMetaObject>
#include <QVariant>
#include <QGenericArgument>
#include <typeinfo>
#include "qxtnullable.h"
#include "qxtglobal.h"
QT_FORWARD_DECLARE_CLASS(QByteArray)
class QxtBoundArgument;
class QxtBoundFunction;

#define QXT_PROTO_10ARGS(T) T p1 = T(), T p2 = T(), T p3 = T(), T p4 = T(), T p5 = T(), T p6 = T(), T p7 = T(), T p8 = T(), T p9 = T(), T p10 = T()
#define QXT_PROTO_9ARGS(T) T p2 = T(), T p3 = T(), T p4 = T(), T p5 = T(), T p6 = T(), T p7 = T(), T p8 = T(), T p9 = T(), T p10 = T()
#define QXT_IMPL_10ARGS(T) T p1, T p2, T p3, T p4, T p5, T p6, T p7, T p8, T p9, T p10

class QXT_CORE_EXPORT QxtGenericFunctionPointer
{
    template<typename FUNCTION>
    friend QxtGenericFunctionPointer qxtFuncPtr(FUNCTION funcPtr);
public:
    QxtGenericFunctionPointer(const QxtGenericFunctionPointer& other)
    {
        funcPtr = other.funcPtr;
        typeName = other.typeName;
    }

    typedef void(voidFunc)();
    voidFunc* funcPtr;
    QByteArray typeName;

protected:
    QxtGenericFunctionPointer(voidFunc* ptr, const QByteArray& typeIdName)
    {
        funcPtr = ptr;
        typeName = typeIdName;
    }
};

template<typename FUNCTION>
QxtGenericFunctionPointer qxtFuncPtr(FUNCTION funcPtr)
{
    return QxtGenericFunctionPointer(reinterpret_cast<QxtGenericFunctionPointer::voidFunc*>(funcPtr), typeid(funcPtr).name());
}

namespace QxtMetaObject
{
    QXT_CORE_EXPORT QByteArray methodName(const char* method);
    QXT_CORE_EXPORT QByteArray methodSignature(const char* method);

    QXT_CORE_EXPORT bool isSignalOrSlot(const char* method);

    QXT_CORE_EXPORT QxtBoundFunction* bind(QObject* recv, const char* invokable, QXT_PROTO_10ARGS(QGenericArgument));
    QXT_CORE_EXPORT QxtBoundFunction* bind(QObject* recv, const char* invokable, QVariant p1, QXT_PROTO_9ARGS(QVariant));
    QXT_CORE_EXPORT bool connect(QObject* sender, const char* signal, QxtBoundFunction* slot,
                                 Qt::ConnectionType type = Qt::AutoConnection);

    QXT_CORE_EXPORT bool invokeMethod(QObject* object, const char* member,
                           const QVariant& arg0 = QVariant(), const QVariant& arg1 = QVariant(),
                           const QVariant& arg2 = QVariant(), const QVariant& arg3 = QVariant(),
                           const QVariant& arg4 = QVariant(), const QVariant& arg5 = QVariant(),
                           const QVariant& arg6 = QVariant(), const QVariant& arg7 = QVariant(),
                           const QVariant& arg8 = QVariant(), const QVariant& arg9 = QVariant());

    QXT_CORE_EXPORT bool invokeMethod(QObject* object, const char* member, Qt::ConnectionType type,
                           const QVariant& arg0 = QVariant(), const QVariant& arg1 = QVariant(),
                           const QVariant& arg2 = QVariant(), const QVariant& arg3 = QVariant(),
                           const QVariant& arg4 = QVariant(), const QVariant& arg5 = QVariant(),
                           const QVariant& arg6 = QVariant(), const QVariant& arg7 = QVariant(),
                           const QVariant& arg8 = QVariant(), const QVariant& arg9 = QVariant());
}

/*!
 * \relates QxtMetaObject
 * Refers to the n'th parameter of QxtBoundFunction::invoke() or of a signal connected to
 * a QxtBoundFunction.
 * \sa QxtMetaObject::bind
 */
#define QXT_BIND(n) QGenericArgument("QxtBoundArgument", reinterpret_cast<void*>(n))

#endif // QXTMETAOBJECT_H
