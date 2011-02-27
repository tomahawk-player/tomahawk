#ifndef SOURCE_H
#define SOURCE_H

#include <QObject>
#include <QSharedPointer>
#include <QVariantMap>

#include "network/dbsyncconnection.h"
#include "collection.h"
#include "typedefs.h"

#include "dllmacro.h"

class DatabaseCommand_LogPlayback;
class ControlConnection;
class FileTransferConnection;

namespace Tomahawk
{

class DLLEXPORT Source : public QObject
{
Q_OBJECT

friend class ::DatabaseCommand_LogPlayback;
friend class ::DBSyncConnection;

public:
    explicit Source( int id, const QString& username = QString() );
    virtual ~Source();

    bool isLocal() const { return m_isLocal; }
    bool isOnline() const { return m_online; }

    QString lastOpGuid() const { return m_lastOpGuid; }

    QString userName() const { return m_username; }
    QString friendlyName() const;
    void setFriendlyName( const QString& fname ) { m_friendlyname = fname; }

    collection_ptr collection() const;
    void addCollection( const Tomahawk::collection_ptr& c );
    void removeCollection( const Tomahawk::collection_ptr& c );

    int id() const { return m_id; }
    ControlConnection* controlConnection() const { return m_cc; }
    void setControlConnection( ControlConnection* cc );

    void scanningProgress( unsigned int files );
    void scanningFinished( unsigned int files );
    
    void setOffline();
    void setOnline();

    unsigned int trackCount() const { return m_stats.value( "numfiles" ).toUInt(); }

    Tomahawk::query_ptr currentTrack() const { return m_currentTrack; }
    QString textStatus() const { return m_textStatus; }

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

    void stateChanged();

public slots:
    void setStats( const QVariantMap& m );

private slots:
    void setLastOpGuid( const QString& guid ) { m_lastOpGuid = guid; }

    void dbLoaded( unsigned int id, const QString& fname );
    void remove();

    void onStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info );
    void onPlaybackStarted( const Tomahawk::query_ptr& query );
    void onPlaybackFinished( const Tomahawk::query_ptr& query );
    
private:
    bool m_isLocal;
    bool m_online;
    QString m_username, m_friendlyname;
    int m_id;
    QList< QSharedPointer<Collection> > m_collections;
    QVariantMap m_stats;
    QString m_lastOpGuid;

    Tomahawk::query_ptr m_currentTrack;
    QString m_textStatus;

    ControlConnection* m_cc;
    FileTransferConnection* m_ftc;
};

};

#endif // SOURCE_H
