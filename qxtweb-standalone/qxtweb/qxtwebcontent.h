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

#ifndef QXTWEBCONTENT_H
#define QXTWEBCONTENT_H

#include <QAbstractSocket>
#include <QByteArray>
#include <QHash>
#include <qxtglobal.h>

class QxtWebContentPrivate;
class QXT_WEB_EXPORT QxtWebContent : public QIODevice
{
    Q_OBJECT
public:
    QxtWebContent(int contentLength, const QByteArray& start, QIODevice* device);
    QxtWebContent(int contentLength, QIODevice* device);
    explicit QxtWebContent(const QByteArray& content, QObject* parent = 0);
    static QHash<QString, QString> parseUrlEncodedQuery(const QString& data);

    virtual qint64 bytesAvailable() const;
    qint64 unreadBytes() const;

    void waitForAllContent();

public Q_SLOTS:
    void ignoreRemainingContent();

protected:
    virtual qint64 readData(char* data, qint64 maxSize);
    virtual qint64 writeData(const char* data, qint64 maxSize);

private Q_SLOTS:
    void errorReceived(QAbstractSocket::SocketError);

private:
    QXT_DECLARE_PRIVATE(QxtWebContent)
};

#endif // QXTWEBCONTENT_H
