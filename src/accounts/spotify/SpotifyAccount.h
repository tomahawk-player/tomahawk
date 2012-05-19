/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#ifndef SpotifyAccount_H
#define SpotifyAccount_H

#include "accounts/ResolverAccount.h"
#include "SourceList.h"
#include "AtticaManager.h"
#include "Playlist.h"
#include "utils/TomahawkUtils.h"
#include "utils/SmartPointerList.h"

class QAction;
class SpotifyPlaylistUpdater;
class QTimer;

class ScriptResolver;

namespace Tomahawk {
namespace Accounts {

class SpotifyAccountConfig;

// metadata for a playlist
struct SpotifyPlaylistInfo {
    QString name, plid, revid;
    bool sync, changed;


    SpotifyPlaylistInfo( const QString& nname, const QString& pid, const QString& rrevid, bool ssync )
        : name( nname ), plid( pid ), revid( rrevid ), sync( ssync ), changed( false ) {}

    SpotifyPlaylistInfo() : sync( false ), changed( false ) {}
};


class SpotifyAccountFactory : public AccountFactory
{
    Q_OBJECT
public:
    SpotifyAccountFactory() {}

    virtual Account* createAccount( const QString& accountId = QString() );
    virtual QString description() const { return tr( "Play music from and sync your playlists with Spotify Premium" ); }
    virtual QString factoryId() const { return "spotifyaccount"; }
    virtual QString prettyName() const { return "Spotify"; }

    virtual AccountTypes types() const { return AccountTypes( ResolverType ); }
    virtual bool allowUserCreation() const { return false; }
    virtual QPixmap icon() const;
    virtual bool isUnique() const { return true; }

};

class SpotifyAccount : public CustomAtticaAccount
{
    Q_OBJECT
public:
    SpotifyAccount( const QString& accountId );
    SpotifyAccount( const QString& accountId, const QString& path );
    virtual ~SpotifyAccount();

    virtual QPixmap icon() const;
    virtual QWidget* configurationWidget();
    virtual QWidget* aboutWidget();
    virtual void saveConfig();
    virtual Attica::Content atticaContent() const;
    virtual void authenticate();
    virtual ConnectionState connectionState() const;
    virtual bool isAuthenticated() const;
    virtual void deauthenticate();

    virtual QWidget* aclWidget() { return 0; }
    virtual Tomahawk::InfoSystem::InfoPluginPtr infoPlugin() { return Tomahawk::InfoSystem::InfoPluginPtr(); }
    virtual SipPlugin* sipPlugin() { return 0; }
    virtual bool preventEnabling() const { return m_preventEnabling; }

    QString sendMessage( const QVariantMap& msg, QObject* receiver = 0, const QString& slot = QString() );

    void registerUpdaterForPlaylist( const QString& plId, SpotifyPlaylistUpdater* updater );
    void unregisterUpdater( const QString& plid );

    bool deleteOnUnsync() const;

    void setManualResolverPath( const QString& resolverPath );

public slots:
    void aboutToShow( QAction* action, const Tomahawk::playlist_ptr& playlist );
    void syncActionTriggered( bool );
    void atticaLoaded(Attica::Content::List);

private slots:
    void resolverChanged();
    void resolverInstalled( const QString& resolverId );

    void resolverMessage( const QString& msgType, const QVariantMap& msg );

    void login( const QString& username, const QString& password );
    // SpotifyResolver message handlers, all take msgtype, msg as argument
  //  void <here>( const QString& msgType, const QVariantMap& msg );
    void startPlaylistSyncWithPlaylist( const QString& msgType, const QVariantMap& msg );
    void playlistCreated( const QString& msgType, const QVariantMap& msg );

    void init();
    void hookupAfterDeletion( bool autoEnable );

private:
    bool checkForResolver();
    void hookupResolver();

    void loadPlaylists();
    void clearUser( bool permanentlyDelete = false );

    void startPlaylistSync( SpotifyPlaylistInfo* playlist );
    void stopPlaylistSync( SpotifyPlaylistInfo* playlist, bool forceDontDelete = false );
    void fetchFullPlaylist( SpotifyPlaylistInfo* playlist );

    void setSyncForPlaylist( const QString& spotifyPlaylistId, bool sync  );

    void createActions();
    void removeActions();

    QWeakPointer<SpotifyAccountConfig> m_configWidget;
    QWeakPointer<QWidget> m_aboutWidget;
    QWeakPointer<ScriptResolver> m_spotifyResolver;

    QMap<QString, QPair<QObject*, QString> > m_qidToSlotMap;

    // List of synced spotify playlists in config UI
    QList< SpotifyPlaylistInfo* > m_allSpotifyPlaylists;
    QHash< QString, SpotifyPlaylistUpdater* > m_updaters;

    QHash< QString, playlist_ptr > m_waitingForCreateReply;

    bool m_preventEnabling;

    SmartPointerList< QAction > m_customActions;
    friend class ::SpotifyPlaylistUpdater;
};

}
}

Q_DECLARE_METATYPE( Tomahawk::Accounts::SpotifyPlaylistInfo* );

#endif // SpotifyAccount_H
