#ifndef SOURCE_H
#define SOURCE_H

#include <QObject>
#include <QSharedPointer>
#include <QVariantMap>

#include "network/dbsyncconnection.h"
#include "collection.h"
#include "typedefs.h"

#include "dllmacro.h"

class ControlConnection;
class FileTransferConnection;

namespace Tomahawk
{

class DLLEXPORT Source : public QObject
{
Q_OBJECT

public:
    explicit Source( const QString& username, ControlConnection* cc );
    explicit Source( const QString& username = QString() );
    virtual ~Source();

    bool isLocal() const { return m_isLocal; }
    bool isOnline() const { return m_online; }

    QString userName() const { return m_username; }
    QString friendlyName() const;
    void setFriendlyName( const QString& fname ) { m_friendlyname = fname; }

    collection_ptr collection() const;
    void addCollection( QSharedPointer<Collection> c );
    void removeCollection( QSharedPointer<Collection> c );

    unsigned int id() const { return m_id; }
    ControlConnection* controlConnection() const { return m_cc; }

    void scanningProgress( unsigned int files ) { emit loadingStateChanged( DBSyncConnection::SCANNING, DBSyncConnection::UNKNOWN, QString::number( files ) ); }

signals:
    void syncedWithDatabase();
    void online();
    void offline();

    void collectionAdded( QSharedPointer<Collection> );
    void collectionRemoved( QSharedPointer<Collection> );

    void stats( const QVariantMap& );
    void usernameChanged( const QString& );

    void playbackStarted( const Tomahawk::query_ptr& query );
    void playbackFinished( const Tomahawk::query_ptr& query );

    // this signal is emitted from DBSyncConnection:
    void loadingStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info );

public slots:
    void doDBSync();
    void setStats( const QVariantMap& m );

protected:
    void setOffline();
    void setOnline();

private slots:
    void dbLoaded( unsigned int id, const QString& fname );
    void remove();

private:
    bool m_isLocal;
    bool m_online;
    QString m_username, m_friendlyname;
    unsigned int m_id;
    QList< QSharedPointer<Collection> > m_collections;
    QVariantMap m_stats;

    ControlConnection* m_cc;
    FileTransferConnection* m_ftc;
};

};

#endif // SOURCE_H
