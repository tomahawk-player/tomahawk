
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
\class QxtAbstractWebService

\inmodule QxtWeb

\brief The QxtAbstractWebService class is a base interface for web services

QxtAbstractWebService provides a common interface for all web service classes.
It uses an event-driven design instead of the more traditional request-response
design used by many web scripting languages. When the user requests a web
page, the service receives a pageRequestedEvent; after the service assembles
the response, it must post a QxtWebPageEvent (or a subclass, such as
QxtWebRedirectEvent or QxtWebErrorEvent).

Usually, an application providing web services will instantiate one
QxtAbstractWebService object for each session, but this is not a requirement.
For services that do not require session management, such as those that serve
only static content, the session factory may return the same pointer for
every invocation, or it may use some more exotic scheme.

When using one service object per session, each service's data members are
independent and may be used to track state across requests. A service object
shared among multiple sessions will retain state across requests as well but
it must implement its own mechanism for separating non-shared data.

The QxtWeb architecture is not multithreaded; that is, QxtAbstractWebService
does not automatically spawn a process for every session. However,
QxtAbstractWebSessionManager performs thread-safe event dispatching, so
subclasses of QxtAbstractWebService are free to create threads themselves.

A web service object may delete itself (see QObject::deleteLater()) to end
the associated session.

\sa QxtAbstractWebSessionManager::ServiceFactory
*/

/*
 * TODO:
 * The current architecture only allows for two behaviors: creating a new session
 * for every connection without a session cookie, or not using sessions at all.
 * This needs to be fixed by adding a function to the session manager to explicitly
 * create a new session.
 */

#include "qxtabstractwebservice.h"

#ifndef QXT_DOXYGEN_RUN
class QxtAbstractWebServicePrivate : public QxtPrivate<QxtAbstractWebService>
{
public:
    QXT_DECLARE_PUBLIC(QxtAbstractWebService)
    QxtAbstractWebServicePrivate() {}

    QxtAbstractWebSessionManager* manager;
};
#endif

/*!
 * Creates a QxtAbstractWebService with the specified \a parent and session \a manager.
 *
 * Often, the session manager will also be the parent, but this is not a requirement.
 *
 * Note that this is an abstract class and cannot be instantiated directly.
 */
QxtAbstractWebService::QxtAbstractWebService(QxtAbstractWebSessionManager* manager, QObject* parent) : QObject(parent)
{
    QXT_INIT_PRIVATE(QxtAbstractWebService);
    qxt_d().manager = manager;
}

/*!
 * Returns the session manager associated with the web service.
 */
QxtAbstractWebSessionManager* QxtAbstractWebService::sessionManager() const
{
    return qxt_d().manager;
}

/*!
 * \fn void QxtAbstractWebService::postEvent(QxtWebEvent* event)
 * Posts an \a event to a web browser that has made a request to the service.
 */

/*!
 * \fn virtual void QxtAbstractWebService::pageRequestedEvent(QxtWebRequestEvent* event)
 * This event handler must be reimplemented in subclasses to receive page
 * request events. The supplied \a event object is owned by the session
 * manager and remains valid until a corresponding response has been
 * processed. It must never be deleted by a service handler.
 *
 * Every page request event received MUST be responded to with a QxtWebPageEvent
 * or a QxtWebPageEvent subclass. This response does not have to be posted
 * during the execution of this function, to support asynchronous design, but
 * failure to post an event will cause the web browser making the request to
 * wait until it times out and leak memory from pending event objects.
 *
 * \sa QxtWebRequestEvent
 */
