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

/*
    One ControlConnection always remains open to each peer.

    They arrange connections/reverse connections, inform us
    when the peer goes offline, and own+setup DBSyncConnections.

*/
#ifndef CONTROLCONNECTION_H
#define CONTROLCONNECTION_H

#include "connection.h"
#include "network/servent.h"
#include "source.h"
#include "typedefs.h"

#include "dllmacro.h"

class FileTransferSession;

class DLLEXPORT ControlConnection : public Connection
{
Q_OBJECT

public:
    explicit ControlConnection( Servent* parent = 0 );
    ~ControlConnection();
    Connection* clone();

    DBSyncConnection* dbSyncConnection();

protected:
    virtual void setup();

protected slots:
    virtual void handleMsg( msg_ptr msg );

signals:

private slots:
    void dbSyncConnFinished( QObject* c );
    void registerSource();
    void onPingTimer();

private:
    void setupDbSyncConnection( bool ondemand = false );

    Tomahawk::source_ptr m_source;
    DBSyncConnection* m_dbsyncconn;

    QString m_dbconnkey;
    bool m_registered;

    QTimer* m_pingtimer;
    QTime m_pingtimer_mark;
};

#endif // CONTROLCONNECTION_H
