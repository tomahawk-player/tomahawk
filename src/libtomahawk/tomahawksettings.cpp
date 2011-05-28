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

#include "tomahawksettings.h"

#ifndef TOMAHAWK_HEADLESS
    #include <QDesktopServices>
    #include "settingsdialog.h"
#endif

#include <QDir>
#include <QDebug>
#include "sip/SipHandler.h"

#define VERSION 3

TomahawkSettings* TomahawkSettings::s_instance = 0;


TomahawkSettings*
TomahawkSettings::instance()
{
    return s_instance;
}


TomahawkSettings::TomahawkSettings( QObject* parent )
    : QSettings( parent )
{
    s_instance = this;

    if( !contains( "configversion") )
    {
        setValue( "configversion", VERSION );
        doInitialSetup();
    }
    else if( value( "configversion" ).toUInt() != VERSION )
    {
        qDebug() << "Config version outdated, old:" << value( "configversion" ).toUInt()
                 << "new:" << VERSION
                 << "Doing upgrade, if any...";

        int current = value( "configversion" ).toUInt();
        while( current < VERSION )
        {
            doUpgrade( current, current + 1 );

            current++;
        }
        // insert upgrade code here as required
        setValue( "configversion", VERSION );
    }
}


TomahawkSettings::~TomahawkSettings()
{
    s_instance = 0;
}

void
TomahawkSettings::doInitialSetup()
{
    // by default we add a local network resolver
    addSipPlugin( "sipzeroconf_autocreated" );
}


void
TomahawkSettings::doUpgrade( int oldVersion, int newVersion )
{
    Q_UNUSED( newVersion );

    if( oldVersion == 1 )
    {
        qDebug() << "Migrating config from verson 1 to 2: script resolver config name";
        if( contains( "script/resolvers" ) ) {
            setValue( "script/loadedresolvers", value( "script/resolvers" ) );
            remove( "script/resolvers" );
        }
    } else if( oldVersion == 2 )
    {
        qDebug() << "Migrating config from version 2 to 3: Converting jabber and twitter accounts to new SIP Factory approach";
        // migrate old accounts to new system. only jabber and twitter, and max one each. create a new plugin for each if needed
        // not pretty as we hardcode a plugin id and assume that we know how the config layout is, but hey, this is migration after all
        if( contains( "jabber/username" ) && contains( "jabber/password" ) )
        {
            QString sipName = "sipjabber";
            if( value( "jabber/username" ).toString().contains( "@gmail" ) )
                sipName = "sipgoogle";

            setValue( QString( "%1_legacy/username" ).arg( sipName ), value( "jabber/username" ) );
            setValue( QString( "%1_legacy/password" ).arg( sipName ), value( "jabber/password" ) );
            setValue( QString( "%1r_legacy/autoconnect" ).arg( sipName ), value( "jabber/autoconnect" ) );
            setValue( QString( "%1_legacy/port" ).arg( sipName ), value( "jabber/port" ) );
            setValue( QString( "%1_legacy/server" ).arg( sipName ), value( "jabber/server" ) );

            addSipPlugin( QString( "%1_legacy" ).arg( sipName ) );

            remove( "jabber/username" );
            remove( "jabber/password" );
            remove( "jabber/autoconnect" );
            remove( "jabber/server" );
            remove( "jabber/port" );
        }
        if( contains( "twitter/ScreenName" ) && contains( "twitter/OAuthToken" ) )
        {
            setValue( "siptwitter_legacy/ScreenName", value( "twitter/ScreenName" ) );
            setValue( "siptwitter_legacy/OAuthToken", value( "twitter/OAuthToken" ) );
            setValue( "siptwitter_legacy/OAuthTokenSecret", value( "twitter/OAuthTokenSecret" ) );
            setValue( "siptwitter_legacy/CachedFriendsSinceID", value( "twitter/CachedFriendsSinceID" ) );
            setValue( "siptwitter_legacy/CachedMentionsSinceID", value( "twitter/CachedMentionsSinceID" ) );
            setValue( "siptwitter_legacy/CachedDirectMessagesSinceID", value( "twitter/CachedDirectMessagesSinceID" ) );
            setValue( "siptwitter_legacy/CachedPeers", value( "twitter/CachedPeers" ) );
            setValue( "siptwitter_legacy/AutoConnect", value( "jabber/autoconnect" ) );

            addSipPlugin( "siptwitter_legacy" );
            remove( "twitter/ScreenName" );
            remove( "twitter/OAuthToken" );
            remove( "twitter/OAuthTokenSecret" );
            remove( "twitter/CachedFriendsSinceID" );
            remove( "twitter/CachedMentionsSinceID" );
            remove( "twitter/CachedDirectMessagesSinceID" );
        }
        // create a zeroconf plugin too
        addSipPlugin( "sipzeroconf_legacy" );
    }
}


QStringList
TomahawkSettings::scannerPaths()
{
    //FIXME: After enough time, remove this hack (and make const)
    #ifndef TOMAHAWK_HEADLESS
    if( value( "scannerpaths" ).isNull() )
        setValue( "scannerpaths", value( "scannerpath" ) );
    return value( "scannerpaths", QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) ).toStringList();
    #else
    if( value( "scannerpaths" ).isNull() )
        setValue( "scannerpaths", value( "scannerpath" ) );
    return value( "scannerpaths", "" ).toStringList();
    #endif
}


void
TomahawkSettings::setScannerPaths( const QStringList& paths )
{
    setValue( "scannerpaths", paths );
}


bool
TomahawkSettings::hasScannerPaths() const
{
    //FIXME: After enough time, remove this hack
    return contains( "scannerpaths" ) || contains( "scannerpath" );
}


TomahawkSettings::ScannerMode
TomahawkSettings::scannerMode() const
{
    return (TomahawkSettings::ScannerMode) value( "scanner-mode", TomahawkSettings::Files ).toInt();
}

 
void
TomahawkSettings::setScannerMode( ScannerMode mode )
{
    setValue( "scanner-mode", mode );
}


uint
TomahawkSettings::scannerTime() const
{
    return value( "scannerTime", 60 ).toUInt();
}


void
TomahawkSettings::setScannerTime( uint time )
{
    setValue( "scannerTime", time );
}


bool
TomahawkSettings::watchForChanges() const
{
    return value( "watchForChanges", false ).toBool();
}


void
TomahawkSettings::setWatchForChanges( bool watch )
{
    setValue( "watchForChanges", watch );
}


void
TomahawkSettings::setAcceptedLegalWarning( bool accept )
{
    setValue( "acceptedLegalWarning", accept );
}

bool
TomahawkSettings::acceptedLegalWarning() const
{
    return value( "acceptedLegalWarning", false ).toBool();
}

bool
TomahawkSettings::httpEnabled() const
{
    return value( "network/http", true ).toBool();
}


void
TomahawkSettings::setHttpEnabled( bool enable )
{
    setValue( "network/http", enable );
}


QString
TomahawkSettings::proxyHost() const
{
    return value( "network/proxy/host", QString() ).toString();
}


void
TomahawkSettings::setProxyHost( const QString& host )
{
    setValue( "network/proxy/host", host );
}


QString
TomahawkSettings::proxyNoProxyHosts() const
{
    return value( "network/proxy/noproxyhosts", QString() ).toString();
}


void
TomahawkSettings::setProxyNoProxyHosts( const QString& hosts )
{
    setValue( "network/proxy/noproxyhosts", hosts );
}


qulonglong
TomahawkSettings::proxyPort() const
{
    return value( "network/proxy/port", 1080 ).toULongLong();
}


void
TomahawkSettings::setProxyPort( const qulonglong port )
{
    setValue( "network/proxy/port", port );
}


QString
TomahawkSettings::proxyUsername() const
{
    return value( "network/proxy/username", QString() ).toString();
}


void
TomahawkSettings::setProxyUsername( const QString& username )
{
    setValue( "network/proxy/username", username );
}


QString
TomahawkSettings::proxyPassword() const
{
    return value( "network/proxy/password", QString() ).toString();
}


void
TomahawkSettings::setProxyPassword( const QString& password )
{
    setValue( "network/proxy/password", password );
}


int
TomahawkSettings::proxyType() const
{
    return value( "network/proxy/type", 0 ).toInt();
}


void
TomahawkSettings::setProxyType( const int type )
{
    setValue( "network/proxy/type", type );
}


bool
TomahawkSettings::proxyDns() const
{
    return value( "network/proxy/dns", false ).toBool();
}


void
TomahawkSettings::setProxyDns( bool lookupViaProxy )
{
    setValue( "network/proxy/dns", lookupViaProxy );
}


QStringList
TomahawkSettings::aclEntries() const
{
    return value( "acl/entries", QStringList() ).toStringList();
}


void
TomahawkSettings::setAclEntries( const QStringList &entries )
{
    setValue( "acl/entries", entries );
}


QByteArray
TomahawkSettings::mainWindowGeometry() const
{
    return value( "ui/mainwindow/geometry" ).toByteArray();
}


void
TomahawkSettings::setMainWindowGeometry( const QByteArray& geom )
{
    setValue( "ui/mainwindow/geometry", geom );
}


QByteArray
TomahawkSettings::mainWindowState() const
{
    return value( "ui/mainwindow/state" ).toByteArray();
}


void
TomahawkSettings::setMainWindowState( const QByteArray& state )
{
    setValue( "ui/mainwindow/state", state );
}


QByteArray
TomahawkSettings::mainWindowSplitterState() const
{
    return value( "ui/mainwindow/splitterState" ).toByteArray();
}


void
TomahawkSettings::setMainWindowSplitterState( const QByteArray& state )
{
    setValue( "ui/mainwindow/splitterState", state );
}


QByteArray
TomahawkSettings::playlistColumnSizes( const QString& playlistid ) const
{
    return value( QString( "ui/playlist/%1/columnSizes" ).arg( playlistid ) ).toByteArray();
}


void
TomahawkSettings::setPlaylistColumnSizes( const QString& playlistid, const QByteArray& state )
{
    setValue( QString( "ui/playlist/%1/columnSizes" ).arg( playlistid ), state );
}


QList<Tomahawk::playlist_ptr>
TomahawkSettings::recentlyPlayedPlaylists() const
{
    QStringList playlist_guids = value( "playlists/recentlyPlayed" ).toStringList();

    QList<Tomahawk::playlist_ptr> playlists;
    foreach( const QString& guid, playlist_guids )
    {
        Tomahawk::playlist_ptr pl = Tomahawk::Playlist::load( guid );
        if ( !pl.isNull() )
            playlists << pl;
    }

    return playlists;
}

QStringList
TomahawkSettings::recentlyPlayedPlaylistGuids( unsigned int amount ) const
{
    QStringList p = value( "playlists/recentlyPlayed" ).toStringList();

    while ( amount && p.count() > (int)amount )
        p.removeAt( p.count() - 1 );

    return p;
}


void
TomahawkSettings::appendRecentlyPlayedPlaylist( const Tomahawk::playlist_ptr& playlist )
{
    QStringList playlist_guids = value( "playlists/recentlyPlayed" ).toStringList();

    playlist_guids.removeAll( playlist->guid() );
    playlist_guids.append( playlist->guid() );

    setValue( "playlists/recentlyPlayed", playlist_guids );

    emit recentlyPlayedPlaylistAdded( playlist );
}

QString
TomahawkSettings::bookmarkPlaylist() const
{
    return value( "playlists/bookmark", QString() ).toString();
}

void
TomahawkSettings::setBookmarkPlaylist( const QString& guid )
{
    setValue( "playlists/bookmark", guid );
}

QStringList
TomahawkSettings::sipPlugins() const
{
    return value( "sip/allplugins", QStringList() ).toStringList();
}

void
TomahawkSettings::setSipPlugins( const QStringList& plugins )
{
    setValue( "sip/allplugins", plugins );
}

QStringList
TomahawkSettings::enabledSipPlugins() const
{
    return value( "sip/enabledplugins", QStringList() ).toStringList();
}

void
TomahawkSettings::setEnabledSipPlugins( const QStringList& list )
{
    setValue( "sip/enabledplugins", list );
}

void
TomahawkSettings::enableSipPlugin( const QString& pluginId )
{
    QStringList list = enabledSipPlugins();
    list << pluginId;
    setEnabledSipPlugins( list );
}

void
TomahawkSettings::disableSipPlugin( const QString& pluginId )
{
    QStringList list = enabledSipPlugins();
    list.removeAll( pluginId );
    setEnabledSipPlugins( list );
}

void
TomahawkSettings::addSipPlugin( const QString& pluginId, bool enable )
{
    QStringList list = sipPlugins();
    list << pluginId;
    setSipPlugins( list );

    if ( enable )
        enableSipPlugin( pluginId );
}

void
TomahawkSettings::removeSipPlugin( const QString& pluginId )
{
    QStringList list = sipPlugins();
    list.removeAll( pluginId );
    setSipPlugins( list );

    if( enabledSipPlugins().contains( pluginId ) )
        disableSipPlugin( pluginId );
}


TomahawkSettings::ExternalAddressMode
TomahawkSettings::externalAddressMode() const
{
    return (TomahawkSettings::ExternalAddressMode) value( "network/external-address-mode", TomahawkSettings::Upnp ).toInt();
}


void
TomahawkSettings::setExternalAddressMode( ExternalAddressMode externalAddressMode )
{
    setValue( "network/external-address-mode", externalAddressMode );
}

bool TomahawkSettings::preferStaticHostPort() const
{
    return value( "network/prefer-static-host-and-port" ).toBool();
}

void TomahawkSettings::setPreferStaticHostPort( bool prefer )
{
    setValue( "network/prefer-static-host-and-port", prefer );
}

QString
TomahawkSettings::externalHostname() const
{
    return value( "network/external-hostname" ).toString();
}

void
TomahawkSettings::setExternalHostname(const QString& externalHostname)
{
    setValue( "network/external-hostname", externalHostname );
}

int
TomahawkSettings::defaultPort() const
{
    return 50210;
}

int
TomahawkSettings::externalPort() const
{
    return value( "network/external-port", 50210 ).toInt();
}

void
TomahawkSettings::setExternalPort(int externalPort)
{
    if ( externalPort == 0 )
        setValue( "network/external-port", 50210);
    else
        setValue( "network/external-port", externalPort);
}


QString
TomahawkSettings::lastFmPassword() const
{
    return value( "lastfm/password" ).toString();
}


void
TomahawkSettings::setLastFmPassword( const QString& password )
{
    setValue( "lastfm/password", password );
}


QByteArray
TomahawkSettings::lastFmSessionKey() const
{
    return value( "lastfm/session" ).toByteArray();
}


void
TomahawkSettings::setLastFmSessionKey( const QByteArray& key )
{
    setValue( "lastfm/session", key );
}


QString
TomahawkSettings::lastFmUsername() const
{
    return value( "lastfm/username" ).toString();
}


void
TomahawkSettings::setLastFmUsername( const QString& username )
{
    setValue( "lastfm/username", username );
}

bool
TomahawkSettings::scrobblingEnabled() const
{
    return value( "lastfm/enablescrobbling", false ).toBool();
}


void
TomahawkSettings::setScrobblingEnabled( bool enable )
{
    setValue( "lastfm/enablescrobbling", enable );
}


QString
TomahawkSettings::xmppBotServer() const
{
    return value( "xmppBot/server", QString() ).toString();
}


void
TomahawkSettings::setXmppBotServer( const QString& server )
{
    setValue( "xmppBot/server", server );
}


QString
TomahawkSettings::xmppBotJid() const
{
    return value( "xmppBot/jid", QString() ).toString();
}


void
TomahawkSettings::setXmppBotJid( const QString& component )
{
    setValue( "xmppBot/jid", component );
}


QString
TomahawkSettings::xmppBotPassword() const
{
    return value( "xmppBot/password", QString() ).toString();
}


void
TomahawkSettings::setXmppBotPassword( const QString& password )
{
    setValue( "xmppBot/password", password );
}


int
TomahawkSettings::xmppBotPort() const
{
    return value( "xmppBot/port", -1 ).toInt();
}


void
TomahawkSettings::setXmppBotPort( const int port )
{
    setValue( "xmppBot/port", port );
}

void
TomahawkSettings::addScriptResolver(const QString& resolver)
{
    setValue( "script/resolvers", allScriptResolvers() << resolver );
}

QStringList
TomahawkSettings::allScriptResolvers() const
{
    return value( "script/resolvers" ).toStringList();
}

void
TomahawkSettings::setAllScriptResolvers( const QStringList& resolver )
{
    setValue( "script/resolvers", resolver );
}


QStringList
TomahawkSettings::enabledScriptResolvers() const
{
    return value( "script/loadedresolvers" ).toStringList();
}

void
TomahawkSettings::setEnabledScriptResolvers( const QStringList& resolvers )
{
    setValue( "script/loadedresolvers", resolvers );
}
