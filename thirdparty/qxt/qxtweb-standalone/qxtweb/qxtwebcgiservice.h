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

#ifndef QXTWEBCGISERVICE_H
#define QXTWEBCGISERVICE_H

#include <QObject>
#include <qxtglobal.h>
#include "qxtabstractwebsessionmanager.h"
#include "qxtabstractwebservice.h"
class QxtWebEvent;
class QxtWebRequestEvent;

class QxtWebCgiServicePrivate;
class QXT_WEB_EXPORT QxtWebCgiService : public QxtAbstractWebService
{
    Q_OBJECT
public:
    QxtWebCgiService(const QString& binary, QxtAbstractWebSessionManager* manager, QObject* parent = 0);

    QString binary() const;
    void setBinary(const QString& bin);

    int timeout() const;
    void setTimeout(int time);

    bool timeoutOverride() const;
    void setTimeoutOverride(bool enable);

    virtual void pageRequestedEvent(QxtWebRequestEvent* event);

private:
    QXT_DECLARE_PRIVATE(QxtWebCgiService)
};

#endif // QXTWEBCGISERVICE_H
