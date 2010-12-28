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
