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
class DatabaseCommand_SocialAction;

namespace Tomahawk
{

class DLLEXPORT Source : public QObject
{
Q_OBJECT

friend class ::DBSyncConnection;
friend class ::ControlConnection;
friend class ::DatabaseCommand_LogPlayback;
friend class ::DatabaseCommand_SocialAction;

public:
    enum AvatarStyle { Original, FancyStyle };

    explicit Source( int id, const QString& username = QString() );
    virtual ~Source();

    bool isLocal() const { return m_isLocal; }
    bool isOnline() const { return m_online; }

    QString userName() const { return m_username; }
    QString friendlyName() const;
    void setFriendlyName( const QString& fname );

    void setAvatar( const QPixmap& avatar );
    QPixmap avatar( AvatarStyle style = Original ) const;

    collection_ptr collection() const;
    void addCollection( const Tomahawk::collection_ptr& c );
    void removeCollection( const Tomahawk::collection_ptr& c );

    int id() const { return m_id; }
    ControlConnection* controlConnection() const { return m_cc; }
    void setControlConnection( ControlConnection* cc );

    void scanningProgress( unsigned int files );
    void scanningFinished( unsigned int files );

    unsigned int trackCount() const;

    Tomahawk::query_ptr currentTrack() const { return m_currentTrack; }
    QString textStatus() const { return m_textStatus; }

    Tomahawk::playlistinterface_ptr getPlaylistInterface();

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

    void socialAttributesChanged();

public slots:
    void setStats( const QVariantMap& m );

private slots:
    void dbLoaded( unsigned int id, const QString& fname );

    void setOffline();
    void setOnline();

    void onStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info );
    void onPlaybackStarted( const Tomahawk::query_ptr& query );
    void onPlaybackFinished( const Tomahawk::query_ptr& query );
    void trackTimerFired();

private:
    void reportSocialAttributesChanged();

    bool m_isLocal;
    bool m_online;
    QString m_username, m_friendlyname;
    int m_id;
    QList< QSharedPointer<Collection> > m_collections;
    QVariantMap m_stats;
    bool m_scrubFriendlyName;

    Tomahawk::query_ptr m_currentTrack;
    QString m_textStatus;
    QTimer m_currentTrackTimer;

    ControlConnection* m_cc;

    QPixmap* m_avatar;
    mutable QPixmap* m_fancyAvatar;

    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

};

#endif // SOURCE_H
