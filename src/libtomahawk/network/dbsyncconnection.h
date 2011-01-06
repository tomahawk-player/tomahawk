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
        SCANNING
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
    void sendOpsData( QString sinceguid, QList< dbop_ptr > ops );
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
