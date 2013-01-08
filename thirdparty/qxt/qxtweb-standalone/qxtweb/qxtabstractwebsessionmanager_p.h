
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

#ifndef QXTABSTRACTWEBSESSIONMANAGER_P_H
#define QXTABSTRACTWEBSESSIONMANAGER_P_H

#include <QObject>
#include <QPointer>
#include <QHash>
#include <QQueue>
#include "qxtabstractwebsessionmanager.h"

#ifndef QXT_DOXYGEN_RUN
class QxtAbstractWebSessionManagerPrivate : public QObject, public QxtPrivate<QxtAbstractWebSessionManager>
{
    Q_OBJECT
public:
    QxtAbstractWebSessionManagerPrivate();
    QXT_DECLARE_PUBLIC(QxtAbstractWebSessionManager)

    QxtAbstractWebSessionManager::ServiceFactory* factory;
    QHash<int, QxtAbstractWebService*> sessions;
    QQueue<int> freeList;
    int maxID;

    int getNextID();

public Q_SLOTS:
    void sessionDestroyed(int sessionID);
};
#endif // QXT_DOXYGEN_RUN

#endif // QXTABSTRACTWEBSESSIONMANAGER_P_H
