/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REMOTEIOCONNECTION_H
#define REMOTEIOCONNECTION_H

#include <QIODevice>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>
#include <QSharedPointer>

#include "controlconnection.h"
#include "filetransfersession.h"

class RemoteIOConnection : public Connection
{
    Q_OBJECT
public:
    RemoteIOConnection(Servent * s, FileTransferSession * fts);
    ~RemoteIOConnection();
    QString id() const;


    void shutdown(bool wait = false);
    void setup();
    void handleMsg(QByteArray msg);
    Connection * clone();

signals:

private slots:
    void sendSome();

private:

    FileTransferSession * m_fts;
    QSharedPointer<QIODevice> m_readdev;
};

#endif // REMOTEIOCONNECTION_H
