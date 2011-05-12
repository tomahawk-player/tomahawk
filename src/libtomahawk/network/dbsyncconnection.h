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

#ifndef DBSYNCCONNECTION_H
#define DBSYNCCONNECTION_H

#include <QObject>
#include <QTimer>
#include <QSharedPointer>
#include <QIODevice>

#include "network/connection.h"
#include "database/op.h"
#include "typedefs.h"

class DBSyncConnection : public Connection
{
Q_OBJECT

public:
    enum State
    {
        UNKNOWN,
        CHECKING,
        FETCHING,
        PARSING,
        SAVING,
        SYNCED,
        SCANNING,
        SHUTDOWN
    };

    explicit DBSyncConnection( Servent* s, Tomahawk::source_ptr src );
    virtual ~DBSyncConnection();

    void setup();
    Connection* clone();

signals:
    void stateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info );

protected slots:
    virtual void handleMsg( msg_ptr msg );

public slots:
    void sendOps();
    /// trigger a re-sync to pick up any new ops
    void trigger();

private slots:
    void gotUs( const QVariantMap& m );
    void gotThemCache( const QVariantMap& m );
    void lastOpApplied();
    void sendOpsData( QString sinceguid, QString lastguid, QList< dbop_ptr > ops );
    void check();
    void idleTimeout();

private:
    void compareAndRequest();
    void synced();
    void changeState( State newstate );

    Tomahawk::source_ptr m_source;
    QVariantMap m_us, m_uscache, m_themcache;
    State m_state;

    QString m_lastSentOp;

    QTimer m_timer;
};

#endif // DBSYNCCONNECTION_H
