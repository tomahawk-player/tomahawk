/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtWeb module of the Qxt library.
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
 * \sa setServiceFactory(ServiceFactory*)
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
 * \fn virtual bool QxtAbstractWebSessionManager::start()
 * Starts the session manager.
 *
 * Session managers should not create sessions before start() is invoked.
 * Subclasses are encouraged to refrain from accepting connections until the
 * session manager is started.
 */

/*!
 * \fn virtual void QxtAbstractWebSessionManager::postEvent(QxtWebEvent* event)
 * Adds the event to the event queue for its associated session.
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
