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

#ifndef QXTWEBCGISERVICE_P_H
#define QXTWEBCGISERVICE_P_H

#include "qxtwebcgiservice.h"
#include <QString>
#include <QHash>
#include <QPair>
#include <QTimer>
#include <QSignalMapper>

#ifndef QXT_DOXYGEN_RUN
QT_FORWARD_DECLARE_CLASS(QProcess)
class QxtWebContent;

struct QxtCgiRequestInfo
{
    QxtCgiRequestInfo();
    QxtCgiRequestInfo(QxtWebRequestEvent* req);
    int sessionID, requestID;
    QHash<QString, QString> headers;
    bool eventSent, terminateSent;
    QTimer* timeout;
};

class QxtWebCgiServicePrivate : public QObject, public QxtPrivate<QxtWebCgiService>
{
    Q_OBJECT
public:
    QXT_DECLARE_PUBLIC(QxtWebCgiService)

    QHash<QProcess*, QxtCgiRequestInfo> requests;
    QHash<QxtWebContent*, QProcess*> processes;
    QString binary;
    int timeout;
    bool timeoutOverride;
    QSignalMapper timeoutMapper;

public Q_SLOTS:
    void browserReadyRead(QObject* o_content = 0);
    void processReadyRead();
    void processFinished();
    void terminateProcess(QObject* o_process);
};
#endif // QXT_DOXYGEN_RUN

#endif // QXTWEBSERVICEDIRECTORY_P_H
