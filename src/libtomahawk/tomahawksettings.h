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

#ifndef TOMAHAWK_SETTINGS_H
#define TOMAHAWK_SETTINGS_h

#include <QSettings>

#include "dllmacro.h"

#include "playlist.h"

/**
 * Convenience wrapper around QSettings for tomahawk-specific config
 */
class DLLEXPORT TomahawkSettings : public QSettings
{
Q_OBJECT

public:
    static TomahawkSettings* instance();

    explicit TomahawkSettings( QObject* parent = 0 );
    virtual ~TomahawkSettings();

    void applyChanges() { emit changed(); }

    /// General settings
    QStringList scannerPath() const; /// QDesktopServices::MusicLocation by default
    void setScannerPath( const QStringList& path );
    bool hasScannerPath() const;
    
    bool acceptedLegalWarning() const;
    void setAcceptedLegalWarning( bool accept );
    
    /// UI settings
    QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry( const QByteArray& geom );
    
    QByteArray mainWindowState() const;
    void setMainWindowState( const QByteArray& state );

    QByteArray mainWindowSplitterState() const;
    void setMainWindowSplitterState( const QByteArray& state );

    /// Playlist stuff
    QByteArray playlistColumnSizes( const QString& playlistid ) const;
    void setPlaylistColumnSizes( const QString& playlistid, const QByteArray& state );

    QList<Tomahawk::playlist_ptr> recentlyPlayedPlaylists() const;
    void appendRecentlyPlayedPlaylist( const Tomahawk::playlist_ptr& playlist );

    /// Jabber settings
    bool jabberAutoConnect() const; /// true by default
    void setJabberAutoConnect( bool autoconnect = false );
    
    QString jabberUsername() const;
    void setJabberUsername( const QString& username );
    
    QString jabberPassword() const;
    void setJabberPassword( const QString& pw );
    
    QString jabberServer() const;
    void setJabberServer( const QString& server );
    
    unsigned int jabberPort() const; // default is 5222
    void setJabberPort( int port );
    
    /// Network settings
    enum ExternalAddressMode { Lan, Upnp };
    ExternalAddressMode externalAddressMode() const;
    void setExternalAddressMode( ExternalAddressMode externalAddressMode );
    
    bool preferStaticHostPort() const;
    void setPreferStaticHostPort( bool prefer );

    bool httpEnabled() const; /// true by default
    void setHttpEnabled( bool enable );

    QString externalHostname() const;
    void setExternalHostname( const QString& externalHostname );

    int externalPort() const;
    void setExternalPort( int externalPort );

    QString proxyHost() const;
    void setProxyHost( const QString &host );

    qulonglong proxyPort() const;
    void setProxyPort( const qulonglong port );

    QString proxyUsername() const;
    void setProxyUsername( const QString &username );

    QString proxyPassword() const;
    void setProxyPassword( const QString &password );

    int proxyType() const;
    void setProxyType( const int type );

    /// Last.fm settings
    bool scrobblingEnabled() const; /// false by default
    void setScrobblingEnabled( bool enable );
    
    QString lastFmUsername() const;
    void setLastFmUsername( const QString& username );
    
    QString lastFmPassword() const;
    void setLastFmPassword( const QString& password );
    
    QByteArray lastFmSessionKey() const;
    void setLastFmSessionKey( const QByteArray& key );
    
    /// Twitter settings
    QString twitterScreenName() const;
    void setTwitterScreenName( const QString& screenName );
    
    QString twitterOAuthToken() const;
    void setTwitterOAuthToken( const QString& oauthtoken );
    
    QString twitterOAuthTokenSecret() const;
    void setTwitterOAuthTokenSecret( const QString& oauthtokensecret );

    qint64 twitterCachedFriendsSinceId() const;
    void setTwitterCachedFriendsSinceId( qint64 sinceid );
    
    qint64 twitterCachedMentionsSinceId() const;
    void setTwitterCachedMentionsSinceId( qint64 sinceid );
    
    qint64 twitterCachedDirectMessagesSinceId() const;
    void setTwitterCachedDirectMessagesSinceId( qint64 sinceid );
    
    QHash<QString, QVariant> twitterCachedPeers() const;
    void setTwitterCachedPeers( const QHash<QString, QVariant> &cachedPeers );
    
    /// XMPP Component Settings
    QString xmppBotServer() const;
    void setXmppBotServer( const QString &server );

    QString xmppBotJid() const;
    void setXmppBotJid( const QString &component );

    QString xmppBotPassword() const;
    void setXmppBotPassword( const QString &password );

    int xmppBotPort() const;
    void setXmppBotPort( const int port );
    
    /// Script resolver settings
    QStringList scriptResolvers() const;
    void setScriptResolvers( const QStringList& resolver );
    void addScriptResolver( const QString& resolver );

signals:
    void changed();

private:
    static TomahawkSettings* s_instance;
};

#endif
