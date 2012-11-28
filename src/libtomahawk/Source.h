/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef SOURCE_H
#define SOURCE_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariantMap>

#include "Typedefs.h"
#include "network/DbSyncConnection.h"
#include "Collection.h"
#include "Query.h"
#include "utils/TomahawkUtils.h"

#include "DllMacro.h"

class ControlConnection;
class DatabaseCommand_LogPlayback;
class DatabaseCommand_SocialAction;
class DatabaseCommand_UpdateSearchIndex;
class DatabaseCommand_DeleteFiles;
class MusicScanner;

namespace Tomahawk
{

class DLLEXPORT Source : public QObject
{
Q_OBJECT

friend class ::DBSyncConnection;
friend class ::ControlConnection;
friend class ::DatabaseCommand_LogPlayback;
friend class ::DatabaseCommand_SocialAction;
friend class ::DatabaseCommand_AddFiles;
friend class ::DatabaseCommand_DeleteFiles;
friend class ::MusicScanner;

public:
    explicit Source( int id, const QString& username = QString() );
    virtual ~Source();

    bool isLocal() const { return m_isLocal; }
    bool isOnline() const { return m_online || m_isLocal; }

    QString userName() const { return m_username; }
    QString friendlyName() const;
    void setFriendlyName( const QString& fname );

#ifndef ENABLE_HEADLESS
    void setAvatar( const QPixmap& avatar );
    QPixmap avatar( TomahawkUtils::ImageMode style = TomahawkUtils::Original, const QSize& size = QSize() );
#endif

    collection_ptr collection() const;
    void addCollection( const Tomahawk::collection_ptr& c );
    void removeCollection( const Tomahawk::collection_ptr& c );

    int id() const { return m_id; }
    ControlConnection* controlConnection() const { return m_cc; }
    void setControlConnection( ControlConnection* cc );

    void scanningProgress( unsigned int files );
    void scanningFinished( bool updateGUI );

    unsigned int trackCount() const;

    Tomahawk::query_ptr currentTrack() const { return m_currentTrack; }
    QString textStatus() const;
    DBSyncConnection::State state() const { return m_state; }

    Tomahawk::playlistinterface_ptr playlistInterface();

signals:
    void syncedWithDatabase();
    void synced();

    void online();
    void offline();

    void collectionAdded( const collection_ptr& collection );
    void collectionRemoved( const collection_ptr& collection );

    void stats( const QVariantMap& );
    void usernameChanged( const QString& );

    void playbackStarted( const Tomahawk::query_ptr& query );
    void playbackFinished( const Tomahawk::query_ptr& query );

    void stateChanged();
    void commandsFinished();

    void socialAttributesChanged( const QString& action );

    void latchedOn( const Tomahawk::source_ptr& to );
    void latchedOff( const Tomahawk::source_ptr& from );

public slots:
    void setStats( const QVariantMap& m );

private slots:
    void dbLoaded( unsigned int id, const QString& fname );
    QString lastCmdGuid() const;
    void updateIndexWhenSynced();

    void setOffline();
    void setOnline();

    void onStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info );

    void onPlaybackStarted( const Tomahawk::query_ptr& query, unsigned int duration );
    void onPlaybackFinished( const Tomahawk::query_ptr& query );
    void trackTimerFired();

    void executeCommands();
    void addCommand( const QSharedPointer<DatabaseCommand>& command );

private:
    void updateTracks();
    void reportSocialAttributesChanged( DatabaseCommand_SocialAction* action );

    QList< QSharedPointer<Collection> > m_collections;
    QVariantMap m_stats;

    bool m_isLocal;
    bool m_online;
    QString m_username;
    QString m_friendlyname;
    int m_id;
    bool m_scrubFriendlyName;
    bool m_updateIndexWhenSynced;
    bool m_avatarUpdated;

    Tomahawk::query_ptr m_currentTrack;
    QString m_textStatus;
    DBSyncConnection::State m_state;
    QTimer m_currentTrackTimer;

    ControlConnection* m_cc;
    QList< QSharedPointer<DatabaseCommand> > m_cmds;
    int m_commandCount;
    QString m_lastCmdGuid;
    mutable QMutex m_cmdMutex;

    mutable QPixmap* m_avatar;
    mutable QPixmap* m_fancyAvatar;
    mutable QByteArray m_avatarHash;
    mutable QHash< TomahawkUtils::ImageMode, QHash< int, QPixmap > > m_coverCache;

    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

} //ns

#endif // SOURCE_H
