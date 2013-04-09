
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
\class QxtWebSlotService

\inmodule QxtWeb

\brief The QxtWebSlotService class provides a Slot based webservice

A WebService that resolves the first part of the path to a slot name and passes the rest as arguments.

\code
class MyService : public QxtWebSlotService
{
// Q_OBJECT
public slots:
    void hello(QxtWebRequestEvent* event, QString a)
    {
        postEvent(new QxtWebPageEvent(event->sessionID, event->requestID, "&lth1&gt"+a.toUtf8()+"&lt/h1&gt));
    }
}
\endcode


/hello/foo<br>
will output<br>
&lth1&gtFoo&lt/h1&gt<br>


\sa QxtAbstractWebService
*/

#include "qxtwebslotservice.h"
#include "qxtwebevent.h"

/*!
    Constructs a new QxtWebSlotService with \a sm and \a parent.
 */
QxtWebSlotService::QxtWebSlotService(QxtAbstractWebSessionManager* sm, QObject* parent): QxtAbstractWebService(sm, parent)
{
}

/*!
    Returns the current absolute url of this service depending on the request \a event.
 */
QUrl QxtWebSlotService::self(QxtWebRequestEvent* event)

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
void QxtWebSlotService::pageRequestedEvent(QxtWebRequestEvent* event)
{
    QList<QString> args = event->url.path().split('/');
    args.removeFirst();
    if (args.at(args.count() - 1).isEmpty())
        args.removeLast();


    ///--------------find action ------------------
    QByteArray action = "index";
    if (args.count())
    {
        action = args.at(0).toUtf8();
        if (action.trimmed().isEmpty())
            action = "index";
        args.removeFirst();
    }



    bool ok = false;
    if (args.count() > 7)
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event),
                                       Q_ARG(QString, args.at(0)),
                                       Q_ARG(QString, args.at(1)),
                                       Q_ARG(QString, args.at(2)),
                                       Q_ARG(QString, args.at(3)),
                                       Q_ARG(QString, args.at(4)),
                                       Q_ARG(QString, args.at(5)),
                                       Q_ARG(QString, args.at(6)),
                                       Q_ARG(QString, args.at(7))
                                      );
    }
    else if (args.count() > 6)
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event),
                                       Q_ARG(QString, args.at(0)),
                                       Q_ARG(QString, args.at(1)),
                                       Q_ARG(QString, args.at(2)),
                                       Q_ARG(QString, args.at(3)),
                                       Q_ARG(QString, args.at(4)),
                                       Q_ARG(QString, args.at(5)),
                                       Q_ARG(QString, args.at(6))
                                      );
    }
    else if (args.count() > 5)
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event),
                                       Q_ARG(QString, args.at(0)),
                                       Q_ARG(QString, args.at(1)),
                                       Q_ARG(QString, args.at(2)),
                                       Q_ARG(QString, args.at(3)),
                                       Q_ARG(QString, args.at(4)),
                                       Q_ARG(QString, args.at(5))
                                      );
    }
    else if (args.count() > 4)
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event),
                                       Q_ARG(QString, args.at(0)),
                                       Q_ARG(QString, args.at(1)),
                                       Q_ARG(QString, args.at(2)),
                                       Q_ARG(QString, args.at(3)),
                                       Q_ARG(QString, args.at(4))
                                      );
    }
    else if (args.count() > 3)
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event),
                                       Q_ARG(QString, args.at(0)),
                                       Q_ARG(QString, args.at(1)),
                                       Q_ARG(QString, args.at(2)),
                                       Q_ARG(QString, args.at(3))
                                      );
    }
    else if (args.count() > 2)
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event),
                                       Q_ARG(QString, args.at(0)),
                                       Q_ARG(QString, args.at(1)),
                                       Q_ARG(QString, args.at(2))
                                      );
    }
    else if (args.count() > 1)
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event),
                                       Q_ARG(QString, args.at(0)),
                                       Q_ARG(QString, args.at(1))
                                      );
    }
    else if (args.count() > 0)
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event),
                                       Q_ARG(QString, args.at(0))
                                      );
    }
    else
    {
        ok = QMetaObject::invokeMethod(this, action,
                                       Q_ARG(QxtWebRequestEvent*, event)
                                      );
    }


    if (!ok)
    {
        QByteArray err = "<h1>Can not find slot</h1> <pre>Class " + QByteArray(metaObject()->className()) + "\r{\npublic slots:\r    void " + action.replace('<', "&lt") + " ( QxtWebRequestEvent* event, ";
        for (int i = 0;i < args.count();i++)
            err += "QString arg" + QByteArray::number(i) + ", ";
        err.chop(2);

        err += " ); \r};\r</pre> ";

        postEvent(new QxtWebErrorEvent(event->sessionID, event->requestID, 404, err));
    }


}

/*!
    \reimp
 */
void QxtWebSlotService::functionInvokedEvent(QxtWebRequestEvent* event)
{
    postEvent(new QxtWebErrorEvent(event->sessionID, event->requestID, 500, "<h1>Not supported</h1>"));
}
