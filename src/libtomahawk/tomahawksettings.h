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
#define TOMAHAWK_SETTINGS_H

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
    enum ScannerMode { Dirs, Files };
    QStringList scannerPaths(); /// QDesktopServices::MusicLocation by default
    void setScannerPaths( const QStringList& paths );
    bool hasScannerPaths() const;
    ScannerMode scannerMode() const;
    void setScannerMode( ScannerMode mode );
    uint scannerTime() const;
    void setScannerTime( uint time );
    
    bool watchForChanges() const;
    void setWatchForChanges( bool watch );

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
    QStringList recentlyPlayedPlaylistGuids( unsigned int amount = 0 ) const;
    void appendRecentlyPlayedPlaylist( const Tomahawk::playlist_ptr& playlist );

    /// SIP plugins
    // all plugins we know about. loaded, unloaded, enabled, disabled.
    void setSipPlugins( const QStringList& plugins );
    QStringList sipPlugins() const;

    void setBookmarkPlaylist( const QString& guid );
    QString bookmarkPlaylist() const;

    // just the enabled sip plugins.
    void setEnabledSipPlugins( const QStringList& list );
    QStringList enabledSipPlugins() const;
    void enableSipPlugin( const QString& pluginId );
    void disableSipPlugin( const QString& pluginId );

    void addSipPlugin( const QString& pluginId, bool enable = true );
    void removeSipPlugin( const QString& pluginId );

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

    int defaultPort() const;
    int externalPort() const;
    void setExternalPort( int externalPort );

    QString proxyHost() const;
    void setProxyHost( const QString &host );
    
    QString proxyNoProxyHosts() const;
    void setProxyNoProxyHosts( const QString &hosts );
    
    qulonglong proxyPort() const;
    void setProxyPort( const qulonglong port );

    QString proxyUsername() const;
    void setProxyUsername( const QString &username );

    QString proxyPassword() const;
    void setProxyPassword( const QString &password );

    int proxyType() const;
    void setProxyType( const int type );

    bool proxyDns() const;
    void setProxyDns( bool lookupViaProxy );

    /// ACL settings
    QStringList aclEntries() const;
    void setAclEntries( const QStringList &entries );

    /// Last.fm settings
    bool scrobblingEnabled() const; /// false by default
    void setScrobblingEnabled( bool enable );

    QString lastFmUsername() const;
    void setLastFmUsername( const QString& username );

    QString lastFmPassword() const;
    void setLastFmPassword( const QString& password );

    QByteArray lastFmSessionKey() const;
    void setLastFmSessionKey( const QByteArray& key );

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
    QStringList allScriptResolvers() const;
    void setAllScriptResolvers( const QStringList& resolvers );
    void addScriptResolver( const QString& resolver );
    QStringList enabledScriptResolvers() const;
    void setEnabledScriptResolvers( const QStringList& resolvers );


signals:
    void changed();
    void recentlyPlayedPlaylistAdded( const Tomahawk::playlist_ptr& playlist );

private:
    void doInitialSetup();
    void doUpgrade( int oldVersion, int newVersion );

    static TomahawkSettings* s_instance;
};

#endif
