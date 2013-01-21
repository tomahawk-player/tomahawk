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
\class QxtWebJsonRPCService

\inmodule QxtWeb

\brief The QxtWebJsonRPCService class provides a Slot based webservice that responds to JSONRPC

See http://json-rpc.org for details on the protocol

To create a Webservice, simply subclass QxtWebJsonRPCService. All slots are exposed as jsonrpc method

\code
class MyService : public QxtWebJsonRPCService
{
Q_OBJECT
public slots:
    int add(int a, int b
    {
        return a +b;
    }
}
\endcode

\code
curl -d '{"method":"add", "id":1, "params": [9,2] }' localhost:1339
{"error":null,"id":1,"result":11}
\endcode

\sa QxtAbstractWebService
*/

#include "qxtwebjsonrpcservice_p.h"
#include "qxtwebevent.h"
#include "qxtwebcontent.h"
#include "qxtjson.h"

#include <QtCore/QMetaMethod>
#include <QtCore/QMetaType>
#include <QtCore/QGenericReturnArgument>
#include <QtCore/QGenericArgument>


QxtWebJsonRPCService::Private::Private(QxtWebJsonRPCService *that)
    : QObject()
    , p(that)
    , invokable(0)
{
}

void QxtWebJsonRPCService::Private::initTables(QObject *in)
{
    invokable = in;
    const QMetaObject *po = invokable->metaObject();
    for (int i = po->methodOffset(); i < po->methodCount(); i++) {

        QxtWebJsonRPCService::Private::Method method;
        QMetaMethod mo = po->method (i);
#if QT_VERSION >=  0x50000
        method.name = QByteArray(mo.methodSignature()).split('(').at(0);
#else
        method.name = QByteArray(mo.signature()).split('(').at(0);
#endif
        method.meta = mo;
        method.argCount = mo.parameterTypes ().count();
        method.returns = QByteArray(mo.typeName()).count();
        methods.insert(method.name + QByteArray::number(method.argCount), method);
    }
}

void QxtWebJsonRPCService::Private::readFinished()
{
    QxtWebContent *c = qobject_cast<QxtWebContent *>(sender());
    handle(c);
}

void QxtWebJsonRPCService::Private::handle(QxtWebContent *c)
{
    QxtWebRequestEvent *event = content.take(c);
    currentRequest = 0;
    c->ignoreRemainingContent();

    QString data = QString::fromUtf8(c->readAll());

    QVariantMap var = QxtJSON::parse(data).toMap();

    if (var.isEmpty()) {
        QByteArray resp = "{\"result\": null, \"error\": \"invalid json data\", \"id\": 0}\r\n";
        QxtWebPageEvent *err = new QxtWebPageEvent(event->sessionID, event->requestID, resp);
        err->status = 500;
        p->postEvent(err);
        return;
    }
    QVariant rid = var.value("id");
    QString action = var.value("method").toString();
    QVariant argsE = var.value("params");
    handle(event, rid, action, argsE);
}

void QxtWebJsonRPCService::Private::handle(QxtWebRequestEvent *event, QVariant rid, QString action,  QVariant argsE)
{
    QVariantMap argsM;
    QVariantList args;

    if (argsE.type() == QVariant::Map) {
        argsM = argsE.toMap();
        args = argsM.values();
    } else {
        args = argsE.toList();
    }

    if (!invokable)
        initTables(p);


    if (!methods.contains(action.toUtf8() + QByteArray::number(args.count()))) {
        QVariantMap res;
        res.insert("result", QVariant());
        res.insert("error", "no such method or incorrect number of arguments");
        res.insert("id", rid);
        QxtWebPageEvent *err = new QxtWebPageEvent(event->sessionID, event->requestID,
                QxtJSON::stringify(res).toUtf8() + "\r\n");
        p->postEvent(err);
        return;
    }
    Method method = methods.value(action.toUtf8() + QByteArray::number(args.count()));
    if (argsE.type() == QVariant::Map) {
        args.clear();
        foreach (const QString &pname, method.meta.parameterNames ()) {
            args.append(argsM.value(pname));
        }
    }


    QList<QGenericArgument> vargs;
    foreach (const QVariant &ar, args) {
        vargs.append(QGenericArgument(ar.typeName(), ar.data()));
    }

    QVariant returnValue;
    currentRequest = event;
    currentRequestId = rid;
    requestCanceled = false;

    bool ok = false;
    if (method.returns) {
        QVariant retA(QVariant::nameToType(method.meta.typeName()));
        QGenericReturnArgument ret(retA.typeName(), retA.data());
        if (args.count() > 8)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4)),
                    (vargs.at(5)),
                    (vargs.at(6)),
                    (vargs.at(7)),
                    (vargs.at(7))
                    );
        }
        if (args.count() > 7)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4)),
                    (vargs.at(5)),
                    (vargs.at(6)),
                    (vargs.at(7))
                    );
        }
        else if (args.count() > 6)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4)),
                    (vargs.at(5)),
                    (vargs.at(6))
                    );
        }
        else if (args.count() > 5)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4)),
                    (vargs.at(5))
                    );
        }
        else if (args.count() > 4)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4))
                    );
        }
        else if (args.count() > 3)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3))
                    );
        }
        else if (args.count() > 2)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2))
                    );
        }
        else if (args.count() > 1)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0)),
                    (vargs.at(1))
                    );
        }
        else if (args.count() > 0)
        {
            ok = method.meta.invoke(invokable,
                    ret,
                    (vargs.at(0))
                    );
        }
        else
        {
            ok = method.meta.invoke(invokable,
                    ret
                    );
        }
        returnValue = retA;
        // the pass through QGenericReturnArgument didn't make it non null.
        returnValue.data_ptr().is_null = 0;
    } else {
        if (args.count() > 8)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4)),
                    (vargs.at(5)),
                    (vargs.at(6)),
                    (vargs.at(7)),
                    (vargs.at(7))
                    );
        }
        if (args.count() > 7)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4)),
                    (vargs.at(5)),
                    (vargs.at(6)),
                    (vargs.at(7))
                    );
        }
        else if (args.count() > 6)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4)),
                    (vargs.at(5)),
                    (vargs.at(6))
                    );
        }
        else if (args.count() > 5)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4)),
                    (vargs.at(5))
                    );
        }
        else if (args.count() > 4)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3)),
                    (vargs.at(4))
                    );
        }
        else if (args.count() > 3)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2)),
                    (vargs.at(3))
                    );
        }
        else if (args.count() > 2)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0)),
                    (vargs.at(1)),
                    (vargs.at(2))
                    );
        }
        else if (args.count() > 1)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0)),
                    (vargs.at(1))
                    );
        }
        else if (args.count() > 0)
        {
            ok = method.meta.invoke(invokable,
                    (vargs.at(0))
                    );
        }
        else
        {
            ok = method.meta.invoke(invokable
                    );
        }
    }
    if (!ok) {
        QVariantMap res;
        res.insert("result", QVariant());
        res.insert("error", "execution failure");
        res.insert("id", rid);
        QxtWebPageEvent *err = new QxtWebPageEvent(event->sessionID, event->requestID,
                QxtJSON::stringify(res).toUtf8() + "\r\n");
        p->postEvent(err);
        return;
    }
    if (requestCanceled) {
        return;
    }

    QVariantMap res;
    res.insert("result", returnValue);
    res.insert("error", QVariant());
    res.insert("id", rid);
    QxtWebPageEvent *err = new QxtWebPageEvent(event->sessionID, event->requestID,
            QxtJSON::stringify(res).toUtf8() + "\r\n");
    p->postEvent(err);
    return;

}
/*!
    Constructs a new QxtWebJsonRPCService with \a sm and \a parent.
 */
QxtWebJsonRPCService::QxtWebJsonRPCService(QxtAbstractWebSessionManager* sm, QObject* parent)
   : QxtAbstractWebService(sm, parent)
   , d(new Private(this))
{

}

QxtWebJsonRPCService::~QxtWebJsonRPCService()
{
    delete d;
}

/*!
 * respond to the current request with an error.
 *
 * The return value of the current slot is NOT used.
 * Instead null is returned, adhering to the jsonrpc specificaiton.
 *
 * Calling this function from somewhere else then a handler slot is undefined behaviour.
 */

void QxtWebJsonRPCService::throwRPCError(QVariant error)
{
    d->requestCanceled= true;
    QxtWebRequestEvent *event = d->currentRequest;

    QVariantMap res;
    res.insert("result", QVariant());
    res.insert("error", error);
    res.insert("id", d->currentRequestId);
    QxtWebPageEvent *err = new QxtWebPageEvent(event->sessionID, event->requestID,
            QxtJSON::stringify(res).toUtf8() + "\r\n");
    postEvent(err);
}

/*!
    Returns the current absolute url of this service depending on the request \a event.
 */
QUrl QxtWebJsonRPCService::self(QxtWebRequestEvent* event)

{
    QStringList  u = event->url.path().split('/');
    QStringList  o = event->originalUrl.path().split('/');
    u.removeFirst();
    o.removeFirst();
    for (int i = 0;i < u.count();i++)
        o.removeLast();


    QString r = "/";
    foreach(const QString& d, o)
    {
        r += d + '/';
    }
    return r;
}

/*!
    \reimp
 */
void QxtWebJsonRPCService::pageRequestedEvent(QxtWebRequestEvent* event)
{
    if (event->method == "GET") {
        QVariantMap params;
        QList<QPair<QByteArray, QByteArray> > enc = event->url.encodedQueryItems ();
        for (int i = 0; i < enc.count(); i++) {
            params.insert(
                    QUrl::fromPercentEncoding (enc.at(i).first),
                    QUrl::fromPercentEncoding (enc.at(i).second));
        }
        QString method = event->url.path().split('/').last();
        d->handle(event, QVariant(), method, params);
        return;
    }


    if (!event->content) {
        QVariantMap res;
        res.insert("result", QVariant());
        res.insert("error", "missing POST data");
        res.insert("id", QVariant());
        QxtWebPageEvent *err = new QxtWebPageEvent(event->sessionID, event->requestID,
                QxtJSON::stringify(res).toUtf8() + "\r\n");
        err->status = 500;
        postEvent(err);
        return;
    }

    d->content.insert(event->content, event);
    if (event->content->bytesNeeded() == 0) {
        d->handle(event->content);
    } else {
        connect(event->content, SIGNAL(readChannelFinished()), d, SLOT(readFinished()));
    }
}

/*!
    \reimp
 */
void QxtWebJsonRPCService::functionInvokedEvent(QxtWebRequestEvent* event)
{
    postEvent(new QxtWebErrorEvent(event->sessionID, event->requestID, 500, "<h1>Not supported</h1>"));
}
