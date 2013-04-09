
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
\class QxtAbstractWebSessionManager

\inmodule QxtWeb

\brief The QxtAbstractWebSessionManager class is a base class for QxtWeb session managers

QxtAbstractWebSessionManager is the base class for all QxtWeb session managers.

Session managers are responsible for managing connections between web browsers
and web services, for creating sessions and their corresponding service objects,
and for managing and dispatching events between browsers and services.

Note that the session manager is not responsible for destroying service objects.
A service object that wishes to end its corresponding session may destroy itself
(see QObject::deleteLater()) and QxtAbstractWebSessionManager will automatically
clean up its internal session tracking data.

\sa QxtAbstractWebService
*/

/*!
 * \typedef QxtAbstractWebSessionManager::ServiceFactory
 * \brief Pointer to a function that generates QxtAbstractWebService objects
 *
 * \bold TYPEDEF: The ServiceFactory type represents a pointer to a function that takes two
 * parameters -- a QxtAbstractWebSessionManager* pointer and an int session ID.
 * The function must return a QxtAbstractWebService* pointer.
 *
 * Usually, an application providing web services will instantiate one
 * QxtAbstractWebService object for each session. For services that do not
 * require session management, such as those that serve only static pages, a
 * single service object may be shared for all requests, or it may use some
 * more exotic scheme. See QxtAbstractWebService for more details.
 */

#include "qxtabstractwebsessionmanager.h"
#include "qxtabstractwebsessionmanager_p.h"
#include "qxtabstractwebservice.h"
#include "qxtmetaobject.h"
#include <QtDebug>

#ifndef QXT_DOXYGEN_RUN
QxtAbstractWebSessionManagerPrivate::QxtAbstractWebSessionManagerPrivate() : factory(0), maxID(1)
{
    // initializers only
}

void QxtAbstractWebSessionManagerPrivate::sessionDestroyed(int sessionID)
{
    if (sessions.contains(sessionID))
    {
        freeList.enqueue(sessionID);
        sessions.remove(sessionID);
	qxt_p().sessionDestroyed(sessionID);
    }
}

int QxtAbstractWebSessionManagerPrivate::getNextID()
{
    if (freeList.empty())
    {
        int next = maxID;
        maxID++;
        return next;
    }
    return freeList.dequeue();
}
#endif

/*!
 * Creates a QxtAbstractWebSessionManager with the specified \a parent.
 *
 * Note that this is an abstract class and cannot be instantiated directly.
 */
QxtAbstractWebSessionManager::QxtAbstractWebSessionManager(QObject* parent) : QObject(parent)
{
    QXT_INIT_PRIVATE(QxtAbstractWebSessionManager);
}

/*!
 * Sets the service \a factory for the session manager.
 *
 * The service factory is invoked every time the session manager creates a new
 * session. Usually, an application providing web services will instantiate one
 * QxtAbstractWebService object for each session. For services that do not
 * require separate sessions, such as those that serve only static pages, the
 * factory may return a pointer to the same object for multiple requests.
 *
 * \sa QxtAbstractWebSessionManager::ServiceFactory
 */
void QxtAbstractWebSessionManager::setServiceFactory(ServiceFactory* factory)
{
    qxt_d().factory = factory;
}

/*!
 * Returns the service factory in use by the session manager.
 *
 * \sa setServiceFactory()
 */
QxtAbstractWebSessionManager::ServiceFactory* QxtAbstractWebSessionManager::serviceFactory() const
{
    return qxt_d().factory;
}

/*!
 * Returns the service object corresponding to the provided \a sessionID.
 */
QxtAbstractWebService* QxtAbstractWebSessionManager::session(int sessionID) const
{
    if (qxt_d().sessions.contains(sessionID))
        return qxt_d().sessions[sessionID];
    return 0;
}

/*!
 * Creates a new session and returns its session ID.
 *
 * This function uses the serviceFactory() to request an instance of the web service.
 * \sa serviceFactory()
 */
int QxtAbstractWebSessionManager::createService()
{
    int sessionID = qxt_d().getNextID();
    if (!qxt_d().factory) return sessionID;

    QxtAbstractWebService* service = serviceFactory()(this, sessionID);
    qxt_d().sessions[sessionID] = service;
    // Using QxtBoundFunction to bind the sessionID to the slot invocation
    QxtMetaObject::connect(service, SIGNAL(destroyed()), QxtMetaObject::bind(&qxt_d(), SLOT(sessionDestroyed(int)), Q_ARG(int, sessionID)), Qt::QueuedConnection);
    return sessionID; // you can always get the service with this
}

/*!
 * Notification that a service has been destroyed. The \a sessionID contains
 * the session ID# which has already been deallocated.
 *
 * Derived classes should reimplement this method to perform any housekeeping
 * chores needed when a service is removed (such as expiring session cookies).
 *
 * This default implementation does nothing at all.
 */
void QxtAbstractWebSessionManager::sessionDestroyed(int)
{
}

/*!
 * \fn virtual bool QxtAbstractWebSessionManager::start()
 * Starts the session manager.
 *
 * Session managers should not create sessions before start() is invoked.
 * Subclasses are encouraged to refrain from accepting connections until the
 * session manager is started.
 *
 * Returns true if the session was successfully started and false otherwise.
 */

/*!
 * \fn virtual bool QxtAbstractWebSessionManager::shutdown()
 * Stops the session manager.
 *
 * This method stops listening for new connections. Any active connections
 * remain viable. It is permissible to start() the session again after a
 * successful shutdown (to change ports for example).
 *
 * Returns true if the session was active (successfully shut down) and false
 * otherwise. This may be connected to an application's aboutToQuit() signal
 * but doing so is not likely to allow any currently processing requests to
 * complete.
 */

/*!
 * \fn virtual void QxtAbstractWebSessionManager::postEvent(QxtWebEvent* event)
 * Adds the \a event to the event queue for its associated session.
 *
 * Since different protocols may require different event processing behavior,
 * there is no default implementation in QxtAbstractWebSessionManager. Subclasses
 * are responsible for maintaining event queues and deciding when and where to
 * dispatch events.
 *
 * Depending on the subclass's implementation posted events may not be dispatched
 * for some time, and is is possible that an event may never be dispatched if
 * the session is terminated before the event is handled.
 *
 * \sa QxtWebEvent
 */

/*!
 * \fn virtual void QxtAbstractWebSessionManager::processEvents()
 * Processes pending events for all sessions.
 *
 * Since different protocols may require different event processing behavior,
 * there is no default implementation in QxtAbstractWebSessionManager. Subclasses
 * are responsible for maintaining event queues and deciding when and where to
 * dispatch events.
 *
 * processEvents() is not required to dispatch all events immediately. In
 * particular, some events may require certain conditions to be met before
 * they may be fully processed. (For example, because HTTP cookies are sent
 * as response headers, QxtHttpServerConnector may not dispatch a
 * QxtWebStoreCookieEvent until a QxtWebPageEvent for the same session is
 * available.) Unprocessed events may remain in the event queue.
 *
 * \sa QxtWebEvent
 */
