/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include <QObject>
#include <QSharedPointer>
#include <QVariantMap>

#include "accounts/AccountManager.h"
#include "collection/Collection.h"
#include "network/DBSyncConnectionState.h"
#include "utils/TomahawkUtils.h"
#include "DllMacro.h"
#include "Typedefs.h"

class ControlConnection;
class DBSyncConnection;
class MusicScanner;

namespace Tomahawk
{
class DatabaseCommand;
class DatabaseCommand_AddFiles;
class DatabaseCommand_DeleteFiles;
class DatabaseCommand_LoadAllSources;
class DatabaseCommand_LogPlayback;
class DatabaseCommand_SocialAction;
class DatabaseCommand_UpdateSearchIndex;

struct PlaybackLog;
class Resolver;
class SourcePrivate;

class DLLEXPORT Source : public QObject
{
Q_OBJECT

friend class ::DBSyncConnection;
friend class ::ControlConnection;
friend class DatabaseCommand_AddFiles;
friend class DatabaseCommand_DeleteFiles;
friend class DatabaseCommand_LoadAllSources;
friend class DatabaseCommand_LogPlayback;
friend class DatabaseCommand_SocialAction;
friend class ::MusicScanner;

public:
    explicit Source( int id, const QString& nodeId = QString() );
    virtual ~Source();

    bool isLocal() const;
    bool isOnline() const;

    QString nodeId() const;

    QString friendlyName() const;
    void setFriendlyName( const QString& fname );

    // fallback when the normal friendlyname from cache is not available
    // this is usually the jabber id or whatever was used when first connected
    QString dbFriendlyName() const;
    void setDbFriendlyName( const QString& dbFriendlyName );

#ifndef ENABLE_HEADLESS
    QPixmap avatar( TomahawkUtils::ImageMode style = TomahawkUtils::Original, const QSize& size = QSize() );
#endif

    collection_ptr dbCollection() const;
    QList< Tomahawk::collection_ptr > collections() const;
    void addCollection( const Tomahawk::collection_ptr& c );
    void removeCollection( const Tomahawk::collection_ptr& c );

    int id() const;
    ControlConnection* controlConnection() const;
    bool setControlConnection( ControlConnection* cc );

    const QSet< Tomahawk::peerinfo_ptr > peerInfos() const;

    void scanningProgress( unsigned int files );
    void scanningFinished( bool updateGUI );

    unsigned int trackCount() const;

    Tomahawk::query_ptr currentTrack() const;
    QString textStatus() const;
    Tomahawk::DBSyncConnectionState state() const;

    Tomahawk::playlistinterface_ptr playlistInterface();

    QSharedPointer<QMutexLocker> acquireLock();

signals:
    void syncedWithDatabase();
    void synced();

    void online();
    void offline();

    void collectionAdded( const Tomahawk::collection_ptr& collection );
    void collectionRemoved( const Tomahawk::collection_ptr& collection );

    void stats( const QVariantMap& );

    void playbackStarted( const Tomahawk::track_ptr& track );
    void playbackFinished( const Tomahawk::track_ptr& track, const Tomahawk::PlaybackLog& log );

    void stateChanged();
    void commandsFinished();

    void socialAttributesChanged( const QString& action );

    void latchedOn( const Tomahawk::source_ptr& to );
    void latchedOff( const Tomahawk::source_ptr& from );

public slots:
    void setStats( const QVariantMap& m );
    QString lastCmdGuid() const;

private slots:
    void setLastCmdGuid( const QString& guid );
    void dbLoaded( unsigned int id, const QString& fname );
    void updateIndexWhenSynced();

    void handleDisconnect( Tomahawk::Accounts::Account*, Tomahawk::Accounts::AccountManager::DisconnectReason reason );
    void setOffline();
    void setOnline();

    void onStateChanged( Tomahawk::DBSyncConnectionState newstate, Tomahawk::DBSyncConnectionState oldstate, const QString& info );

    void onPlaybackStarted( const Tomahawk::track_ptr& track, unsigned int duration );
    void onPlaybackFinished( const Tomahawk::track_ptr& track, const Tomahawk::PlaybackLog& log );
    void trackTimerFired();

    void executeCommands();
    void addCommand( const dbcmd_ptr& command );

private:
    Q_DECLARE_PRIVATE( Source )
    SourcePrivate* d_ptr;

    static bool friendlyNamesLessThan( const QString& first, const QString& second ); //lessThan for sorting

    void updateTracks();
    void reportSocialAttributesChanged( DatabaseCommand_SocialAction* action );
};

} //ns

#endif // SOURCE_H
