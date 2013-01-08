
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

#ifndef QXTBOUNDFUNCTION_H
#define QXTBOUNDFUNCTION_H

#include <QObject>
#include <QMetaObject>
#include <QGenericArgument>
#include <qxtmetaobject.h>
#include <qxtnull.h>
#include <QThread>
#include <QtDebug>

/*!
\class QxtBoundFunction

\inmodule QxtCore

\brief Binds parameters to a function call

 * A bound function is very similar to what the C++ FAQ Lite refers to as "functionoids."
 * (http://www.parashift.com/c++-faq-lite/pointers-to-members.html#faq-33.10)
 * It is similar in use to a function pointer, but allows any or all parameters to be
 * pre-filled with constant values. The remaining parameters are specified when the
 * function is invoked, for instance, by a Qt signal connection.
 *
 * By far, the most common expected use is to provide a parameter to a slot when the
 * signal doesn't have offer one. Many developers new to Qt try to write code like this:
 * \code
 *     connect(button, SIGNAL(clicked()), lineEdit, SLOT(setText("Hello, world")));
 * \endcode
 * Experienced Qt developers will immediately spot the flaw here. The typical solution
 * is to create a short, one-line wrapper slot that invokes the desired function. Some
 * clever developers may even use QSignalMapper to handle slots that only need one
 * int or QString parameter.
 *
 * QxtBoundFunction enables the previous connect statement to be written like this:
 * \code
 *     connect(button, SIGNAL(clicked()), QxtMetaObject::bind(lineEdit, SLOT(setText(QString)), Q_ARG(QString, "Hello, world!")));
 * \code
 * This accomplishes the same result without having to create a new slot, or worse,
 * an entire object, just to pass a constant value.
 *
 * Additionally, through the use of the QXT_BIND macro, parameters from the signal
 * can be rearranged, skipped, or passed alongside constant arguments provided
 * with the Q_ARG macro. This can be used to provide stateful callbacks to a
 * generic function, for example.
 *
 * Many kinds of functions can be bound. The most common binding applies to
 * Qt signals and slots, but standard C/C++ functions can be bound as well.
 * Future development may add the ability to bind to C++ member functions,
 * and developers can make custom QxtBoundFunction subclasses for even more
 * flexibility if necessary.
 *
 *
 */
class QXT_CORE_EXPORT QxtBoundFunction : public QObject
{
    Q_OBJECT
public:
#ifndef QXT_DOXYGEN_RUN
    virtual ~QxtBoundFunction() {};
#endif

    /*!
     * Invokes the bound function and returns a value.
     *
     * The template parameter should be the return type of the invoked function. This overload accepts
     * QVariant parameters and will guess the data type of each parameter based on the type of the QVariant.
     */
    template <class T>
    inline QxtNullable<T> invoke(QXT_PROTO_10ARGS(QVariant))
    {
        if (!parent() || QThread::currentThread() == parent()->thread())
            return invoke<T>(Qt::DirectConnection, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
#if QT_VERSION >= 0x040300
        return invoke<T>(Qt::BlockingQueuedConnection, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
#else
        qWarning() << "QxtBoundFunction::invoke: Cannot return a value using a queued connection";
        return QxtNull();
#endif
    }

    /*!
     * Invokes the bound function and returns a value.
     *
     * The template parameter should be the return type of the invoked function. This overload accepts
     * QGenericArgument parameters, expressed using the Q_ARG() macro.
     */
    template <class T>
    QxtNullable<T> invoke(Qt::ConnectionType type, QVariant p1, QXT_PROTO_9ARGS(QVariant))
    {
        if (type == Qt::QueuedConnection)
        {
            qWarning() << "QxtBoundFunction::invoke: Cannot return a value using a queued connection";
            return QxtNull();
        }
        T retval;
        // I know this is a totally ugly function call
        if (invoke(type, QGenericReturnArgument(qVariantFromValue<T>(*reinterpret_cast<T*>(0)).typeName(), reinterpret_cast<void*>(&retval)),
                   p1, p2, p3, p4, p5, p6, p7, p8, p9, p10))
        {
            return retval;
        }
        else
        {
            return QxtNull();
        }
    }

    /*!
     * Invokes the bound function, discarding the return value.
     *
     * This overload accepts QVariant parameters and will guess the data type of each
     * parameter based on the type of the QVariant.
     *
     * This function returns true if the invocation was successful, otherwise it
     * returns false.
     */
    inline bool invoke(QVariant p1, QXT_PROTO_9ARGS(QVariant))
    {
        return invoke(Qt::AutoConnection, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
    /*!
     * Invokes the bound function, discarding the return value.
     *
     * This overload accepts QVariant parameters and will guess the data type of each
     * parameter based on the type of the QVariant. It also allows you to specify the
     * connection type, allowing the bound function to be invoked across threads using
     * the Qt event loop.
     *
     * This function returns true if the invocation was successful, otherwise it
     * returns false.
     */
    bool invoke(Qt::ConnectionType, QVariant p1, QXT_PROTO_9ARGS(QVariant));

    /*!
     * Invokes the bound function, discarding the return value.
     *
     * This overload accepts QGenericArgument parameters, expressed using the Q_ARG()
     * macro.
     *
     * This function returns true if the invocation was successful, otherwise it
     * returns false.
     */
    inline bool invoke(QXT_PROTO_10ARGS(QGenericArgument))
    {
        return invoke(Qt::AutoConnection, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
    /*!
     * Invokes the bound function, discarding the return value.
     *
     * This overload accepts QGenericArgument parameters, expressed using the Q_ARG()
     * macro. It also allows you to specify the connection type, allowing the bound
     * function to be invoked across threads using the Qt event loop.
     *
     * This function returns true if the invocation was successful, otherwise it
     * returns false.
     */
    inline bool invoke(Qt::ConnectionType type, QXT_PROTO_10ARGS(QGenericArgument))
    {
        return invoke(type, QGenericReturnArgument(), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }

    /*!
     * Invokes the bound function and assigns the return value to a parameter passed by reference.
     *
     * Use the Q_RETURN_ARG() macro to pass a reference to an assignable object of the function's
     * return type. When the function completes, its return value will be stored in that object.
     *
     * This overload accepts QVariant parameters and will guess the data type of each
     * parameter based on the type of the QVariant.
     *
     * This function returns true if the invocation was successful, otherwise it
     * returns false.
     */
    inline bool invoke(QGenericReturnArgument returnValue, QVariant p1, QXT_PROTO_9ARGS(QVariant))
    {
        return invoke(Qt::AutoConnection, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
    /*!
     * Invokes the bound function and assigns the return value to a parameter passed by reference.
     *
     * Use the Q_RETURN_ARG() macro to pass a reference to an assignable object of the function's
     * return type. When the function completes, its return value will be stored in that object.
     *
     * This overload accepts QVariant parameters and will guess the data type of each
     * parameter based on the type of the QVariant. It also allows you to specify the
     * connection type, allowing the bound function to be invoked across threads using
     * the Qt event loop.
     *
     * This function returns true if the invocation was successful, otherwise it
     * returns false.
     */
    bool invoke(Qt::ConnectionType type, QGenericReturnArgument returnValue, QVariant p1, QXT_PROTO_9ARGS(QVariant));

    /*!
     * Invokes the bound function and assigns the return value to a parameter passed by reference.
     *
     * Use the Q_RETURN_ARG() macro to pass a reference to an assignable object of the function's
     * return type. When the function completes, its return value will be stored in that object.
     *
     * This overload accepts QGenericArgument parameters, expressed using the Q_ARG()
     * macro.
     *
     * This function returns true if the invocation was successful, otherwise it
     * returns false.
     */
    inline bool invoke(QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QGenericArgument))
    {
        return invoke(Qt::AutoConnection, returnValue, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }
    /*!
     * Invokes the bound function and assigns the return value to a parameter passed by reference.
     *
     * Use the Q_RETURN_ARG() macro to pass a reference to an assignable object of the function's
     * return type. When the function completes, its return value will be stored in that object.
     *
     * This overload accepts QGenericArgument parameters, expressed using the Q_ARG()
     * macro. It also allows you to specify the connection type, allowing the bound
     * function to be invoked across threads using the Qt event loop.
     *
     * This function returns true if the invocation was successful, otherwise it
     * returns false.
     */
    bool invoke(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QGenericArgument));

protected:
#ifndef QXT_DOXYGEN_RUN
    QxtBoundFunction(QObject* parent = 0);
#endif

    /*!
     * Performs the work of invoking the bound function.
     *
     * This function is pure virtual. The various QxtMetaObject::bind() functions return opaque subclasses
     * of QxtBoundFunction. If you wish to create a new kind of bound function, reimplement this function to
     * perform the invocation and assign the function's return value, if any, to the returnValue parameter.
     *
     * This function should return true if the invocation is successful and false if an error occurs.
     */
    virtual bool invokeImpl(Qt::ConnectionType type, QGenericReturnArgument returnValue, QXT_PROTO_10ARGS(QGenericArgument)) = 0;
};

#endif
