
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
\namespace QxtMetaObject

\inmodule QxtCore

\brief The QxtMetaObject namespace provides extensions to QMetaObject

including QxtMetaObject::bind

*/

#include "qxtmetaobject.h"
#include "qxtboundfunction.h"
#include "qxtboundcfunction.h"
#include "qxtmetatype.h"

#include <QByteArray>
#include <QMetaObject>
#include <QMetaMethod>
#include <QtDebug>

#ifndef QXT_DOXYGEN_RUN
class QxtBoundArgument
{
    // This class intentionally left blank
};
Q_DECLARE_METATYPE(QxtBoundArgument)

class QxtBoundFunctionBase;

QxtBoundFunction::QxtBoundFunction(QObject* parent) : QObject(parent)
{
    // initializer only
}
#endif

bool QxtBoundFunction::invoke(Qt::ConnectionType type, QXT_IMPL_10ARGS(QVariant))
{
    return invoke(type, QXT_VAR_ARG(1), QXT_VAR_ARG(2), QXT_VAR_ARG(3), QXT_VAR_ARG(4), QXT_VAR_ARG(5), QXT_VAR_ARG(6), QXT_VAR_ARG(7), QXT_VAR_ARG(8), QXT_VAR_ARG(9), QXT_VAR_ARG(10));
}

bool QxtBoundFunction::invoke(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QVariant))
{
    return invoke(type, returnValue, QXT_VAR_ARG(1), QXT_VAR_ARG(2), QXT_VAR_ARG(3), QXT_VAR_ARG(4), QXT_VAR_ARG(5), QXT_VAR_ARG(6), QXT_VAR_ARG(7), QXT_VAR_ARG(8), QXT_VAR_ARG(9), QXT_VAR_ARG(10));
}

QxtBoundFunctionBase::QxtBoundFunctionBase(QObject* parent, QGenericArgument* params[10], QByteArray types[10]) : QxtBoundFunction(parent)
{
    for (int i = 0; i < 10; i++)
    {
        if (!params[i]) break;
        if (QByteArray(params[i]->name()) == "QxtBoundArgument")
        {
            arg[i] = QGenericArgument("QxtBoundArgument", params[i]->data());
        }
        else
        {
            data[i] = qxtConstructFromGenericArgument(*params[i]);
            arg[i] = p[i] = QGenericArgument(params[i]->name(), data[i]);
        }
        bindTypes[i] = types[i];
    }
}

QxtBoundFunctionBase::~QxtBoundFunctionBase()
{
    for (int i = 0; i < 10; i++)
    {
        if (arg[i].name() == 0) return;
        if (QByteArray(arg[i].name()) != "QxtBoundArgument") qxtDestroyFromGenericArgument(arg[i]);
    }
}

int QxtBoundFunctionBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod)
    {
        if (_id == 0)
        {
            for (int i = 0; i < 10; i++)
            {
                if (QByteArray(arg[i].name()) == "QxtBoundArgument")
                {
                    p[i] = QGenericArgument(bindTypes[i].constData(), _a[(quintptr)(arg[i].data())]);
                }
            }
            invokeImpl(Qt::DirectConnection, QGenericReturnArgument(), p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
        }
        _id = -1;
    }
    return _id;
}

bool QxtBoundFunctionBase::invokeBase(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QGenericArgument))
{
    QGenericArgument* args[10] = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10 };
    for (int i = 0; i < 10; i++)
    {
        if (QByteArray(arg[i].name()) == "QxtBoundArgument")
        {
            p[i] = *args[(quintptr)(arg[i].data()) - 1];
        }
    }
    return invokeImpl(type, returnValue, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
}

bool QxtBoundFunction::invoke(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QGenericArgument))
{
    return reinterpret_cast<QxtBoundFunctionBase*>(this)->invokeBase(type, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

#ifndef QXT_DOXYGEN_RUN
class QxtBoundSlot : public QxtBoundFunctionBase
{
public:
    QByteArray sig;

    QxtBoundSlot(QObject* receiver, const char* invokable, QGenericArgument* params[10], QByteArray types[10]) : QxtBoundFunctionBase(receiver, params, types), sig(invokable)
    {
        // initializers only
    }

    virtual bool invokeImpl(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_IMPL_10ARGS(QGenericArgument))
    {
        if (!QMetaObject::invokeMethod(parent(), QxtMetaObject::methodName(sig.constData()), type, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10))
        {
            qWarning() << "QxtBoundFunction: call to" << sig << "failed";
            return false;
        }
        return true;
    }
};
#endif

namespace QxtMetaObject
{

    /*!
        Returns the name of the given method.

        Example usage:
        \code
        QByteArray method = QxtMetaObject::methodName(" int foo ( int bar, double baz )");
        // method is now "foo"
        \endcode
     */
    QByteArray methodName(const char* method)
    {
        QByteArray name = methodSignature(method);
        const int idx = name.indexOf("(");
        if (idx != -1)
            name.truncate(idx);
        return name;
    }

    /*!
        Returns the signature of the given method.
     */
    QByteArray methodSignature(const char* method)
    {
        QByteArray name = QMetaObject::normalizedSignature(method);
        if(name[0] >= '0' && name[0] <= '9')
            return name.mid(1);
        return name;
    }

    /*!
        Checks if \a method contains parentheses and begins with 1 or 2.
     */
    bool isSignalOrSlot(const char* method)
    {
        QByteArray m(method);
        return (m.count() && (m[0] >= '0' && m[0] <= '9') && m.contains('(') && m.contains(')'));
    }

    /*!
     * Creates a binding to the provided signal, slot, or Q_INVOKABLE method using the
     * provided parameter list. The type of each argument is deduced from the type of
     * the QVariant. This function cannot bind positional arguments; see the
     * overload using QGenericArgument.
     *
     * If the provided QObject does not implement the requested method, or if the
     * argument list is incompatible with the method's function signature, this
     * function returns NULL.
     *
     * The returned QxtBoundFunction is created as a child of the receiver.
     * Changing the parent will result in undefined behavior.
     *
     * \sa QxtMetaObject::connect, QxtBoundFunction
     */
    QxtBoundFunction* bind(QObject* recv, const char* invokable, QXT_IMPL_10ARGS(QVariant))
    {
        if (!recv)
        {
            qWarning() << "QxtMetaObject::bind: cannot connect to null QObject";
            return 0;
        }

        QVariant* args[10] = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10 };
        QByteArray connSlot("2"), recvSlot(QMetaObject::normalizedSignature(invokable));
        const QMetaObject* meta = recv->metaObject();
        int methodID = meta->indexOfMethod(QxtMetaObject::methodSignature(recvSlot.constData()));
        if (methodID == -1)
        {
            qWarning() << "QxtMetaObject::bind: no such method " << recvSlot;
            return 0;
        }
        QMetaMethod method = meta->method(methodID);
        int argCount = method.parameterTypes().count();
        const QList<QByteArray> paramTypes = method.parameterTypes();

        for (int i = 0; i < argCount; i++)
        {
            if (paramTypes[i] == "QxtBoundArgument") continue;
            int type = QMetaType::type(paramTypes[i].constData());
            if (!args[i]->canConvert((QVariant::Type)type))
            {
                qWarning() << "QxtMetaObject::bind: incompatible parameter list for " << recvSlot;
                return 0;
            }
        }

        return QxtMetaObject::bind(recv, invokable, QXT_ARG(1), QXT_ARG(2), QXT_ARG(3), QXT_ARG(4), QXT_ARG(5), QXT_ARG(6), QXT_ARG(7), QXT_ARG(8), QXT_ARG(9), QXT_ARG(10));
    }

    /*!
     * Creates a binding to the provided signal, slot, or Q_INVOKABLE method using the
     * provided parameter list. Use the Q_ARG macro to specify constant parameters, or
     * use the QXT_BIND macro to relay a parameter from a connected signal or passed
     * via the QxtBoundFunction::invoke() method.
     *
     * If the provided QObject does not implement the requested method, or if the
     * argument list is incompatible with the method's function signature, this
     * function returns NULL.
     *
     * The returned QxtBoundFunction is created as a child of the receiver.
     * Changing the parent will result in undefined behavior.
     *
     * \sa QxtMetaObject::connect, QxtBoundFunction, QXT_BIND
     */
    QxtBoundFunction* bind(QObject* recv, const char* invokable, QXT_IMPL_10ARGS(QGenericArgument))
    {
        if (!recv)
        {
            qWarning() << "QxtMetaObject::bind: cannot connect to null QObject";
            return 0;
        }

        QGenericArgument* args[10] = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10 };
        QByteArray connSlot("2"), recvSlot(QMetaObject::normalizedSignature(invokable)), bindTypes[10];
        const QMetaObject* meta = recv->metaObject();
        int methodID = meta->indexOfMethod(QxtMetaObject::methodSignature(recvSlot.constData()).constData());
        if (methodID == -1)
        {
            qWarning() << "QxtMetaObject::bind: no such method " << recvSlot;
            return 0;
        }
        QMetaMethod method = meta->method(methodID);
        int argCount = method.parameterTypes().count();

        connSlot += QxtMetaObject::methodName(invokable) + '(';
        for (int i = 0; i < 10; i++)
        {
            if (args[i]->name() == 0) break;        // done
            if (i >= argCount)
            {
                qWarning() << "QxtMetaObject::bind: too many arguments passed to " << invokable;
                return 0;
            }
            if (i > 0) connSlot += ',';             // argument separator
            if (QByteArray(args[i]->name()) == "QxtBoundArgument")
            {
                Q_ASSERT_X((quintptr)(args[i]->data()) > 0 && (quintptr)(args[i]->data()) <= 10, "QXT_BIND", "invalid argument number");
                connSlot += method.parameterTypes()[i];
                bindTypes[i] = method.parameterTypes()[i];
            }
            else
            {
                connSlot += args[i]->name();        // type name
            }
        }
        connSlot = QMetaObject::normalizedSignature(connSlot += ')');

        if (!QMetaObject::checkConnectArgs(recvSlot.constData(), connSlot.constData()))
        {
            qWarning() << "QxtMetaObject::bind: provided parameters " << connSlot.mid(connSlot.indexOf('(')) << " is incompatible with " << invokable;
            return 0;
        }

        return new QxtBoundSlot(recv, invokable, args, bindTypes);
    }

    /*!
        Connects a signal to a QxtBoundFunction.
     */
    bool connect(QObject* sender, const char* signal, QxtBoundFunction* slot, Qt::ConnectionType type)
    {
        Q_ASSERT(sender);
        const QMetaObject* meta = sender->metaObject();
        int methodID = meta->indexOfMethod(meta->normalizedSignature(signal).mid(1).constData());
        if (methodID < 0)
        {
            qWarning() << "QxtMetaObject::connect: no such signal: " << QByteArray(signal).mid(1);
            return false;
        }

        return QMetaObject::connect(sender, methodID, slot, QObject::staticMetaObject.methodCount(), (int)(type));
    }

    /*!
        \relates QxtMetaObject
        This overload always invokes the member using the connection type Qt::AutoConnection.

        \sa QMetaObject::invokeMethod()
     */
    bool invokeMethod(QObject* object, const char* member, const QVariant& arg0,
                      const QVariant& arg1, const QVariant& arg2, const QVariant& arg3,
                      const QVariant& arg4, const QVariant& arg5, const QVariant& arg6,
                      const QVariant& arg7, const QVariant& arg8, const QVariant& arg9)
    {
        return invokeMethod(object, member, Qt::AutoConnection,
                            arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }

    /*!
        \relates QxtMetaObject

        Invokes the \a member (a signal or a slot name) on the \a object.
        Returns \c true if the member could be invoked. Returns \c false
        if there is no such member or the parameters did not match.

        \sa QMetaObject::invokeMethod()
     */
    bool invokeMethod(QObject* object, const char* member, Qt::ConnectionType type,
                      const QVariant& arg0, const QVariant& arg1, const QVariant& arg2,
                      const QVariant& arg3, const QVariant& arg4, const QVariant& arg5,
                      const QVariant& arg6, const QVariant& arg7, const QVariant& arg8, const QVariant& arg9)
    {
        #define QXT_MO_ARG(i) QGenericArgument(arg ## i.typeName(), arg ## i.constData())
        return QMetaObject::invokeMethod(object, methodName(member), type,
                                         QXT_MO_ARG(0), QXT_MO_ARG(1), QXT_MO_ARG(2), QXT_MO_ARG(3), QXT_MO_ARG(4),
                                         QXT_MO_ARG(5), QXT_MO_ARG(6), QXT_MO_ARG(7), QXT_MO_ARG(8), QXT_MO_ARG(9));
    }
}
