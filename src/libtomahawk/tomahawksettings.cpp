#include "tomahawksettings.h"

#ifndef TOMAHAWK_HEADLESS
    #include <QDesktopServices>
    #include "settingsdialog.h"
#endif

#include <QDir>
#include <QDebug>

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

    #ifndef TOMAHAWK_HEADLESS
    if( !contains( "configversion") )
    {
        setValue( "configversion", SettingsDialog::VERSION );
    }
    else if( value( "configversion" ).toUInt() != SettingsDialog::VERSION )
    {
        qDebug() << "Config version outdated, old:" << value( "configversion" ).toUInt()
                 << "new:" << SettingsDialog::VERSION
                 << "Doing upgrade, if any...";
        
        // insert upgrade code here as required
        setValue( "configversion", SettingsDialog::VERSION );
    }
    #endif
}


TomahawkSettings::~TomahawkSettings()
{
    s_instance = 0;
}


QString
TomahawkSettings::scannerPath() const
{
    #ifndef TOMAHAWK_HEADLESS
    return value( "scannerpath", QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) ).toString();
    #else
    return value( "scannerpath", "" ).toString();
    #endif
}


void
TomahawkSettings::setScannerPath( const QString& path )
{
    setValue( "scannerpath", path );
}


bool
TomahawkSettings::hasScannerPath() const
{
    return contains( "scannerpath" );
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


QList<QVariant>
TomahawkSettings::playlistColumnSizes( const QString& playlistid ) const
{
    return value( QString( "ui/playlist/%1/columnSizes" ).arg( playlistid ) ).toList();
}


void
TomahawkSettings::setPlaylistColumnSizes( const QString& playlistid, const QList<QVariant>& cols )
{
    setValue( QString( "ui/playlist/%1/columnSizes" ).arg( playlistid ), cols );
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


void
TomahawkSettings::appendRecentlyPlayedPlaylist( const Tomahawk::playlist_ptr& playlist )
{
    QStringList playlist_guids = value( "playlists/recentlyPlayed" ).toStringList();

    playlist_guids.removeAll( playlist->guid() );
    playlist_guids.append( playlist->guid() );

    setValue( "playlists/recentlyPlayed", playlist_guids );
}


bool
TomahawkSettings::jabberAutoConnect() const
{
    return value( "jabber/autoconnect", true ).toBool();
}


void
TomahawkSettings::setJabberAutoConnect( bool autoconnect )
{
    setValue( "jabber/autoconnect", autoconnect );
}


unsigned int
TomahawkSettings::jabberPort() const
{
    return value( "jabber/port", 5222 ).toUInt();
}


void
TomahawkSettings::setJabberPort( int port )
{
    if ( port < 0 )
      return;
    setValue( "jabber/port", port );
}


QString
TomahawkSettings::jabberServer() const
{
    return value( "jabber/server" ).toString();
}


void
TomahawkSettings::setJabberServer( const QString& server )
{
    setValue( "jabber/server", server );
}


QString
TomahawkSettings::jabberUsername() const
{
    return value( "jabber/username" ).toString();
}


void
TomahawkSettings::setJabberUsername( const QString& username )
{
    setValue( "jabber/username", username );
}


QString
TomahawkSettings::jabberPassword() const
{
    return value( "jabber/password" ).toString();
}


void
TomahawkSettings::setJabberPassword( const QString& pw )
{
    setValue( "jabber/password", pw );
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
TomahawkSettings::externalPort() const
{
    return value( "network/external-port" ).toInt();
}

void
TomahawkSettings::setExternalPort(int externalPort)
{
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

QString
TomahawkSettings::twitterOAuthToken() const
{
    return value( "twitter/oauthtoken" ).toString();
}

void
TomahawkSettings::setTwitterOAuthToken( const QString& oauthtoken )
{
    setValue( "twitter/username", oauthtoken );
}

QString
TomahawkSettings::twitterOAuthTokenSecret() const
{
    return value( "twitter/oauthtokensecret" ).toString();
}

void
TomahawkSettings::setTwitterOAuthTokenSecret( const QString& oauthtokensecret )
{
    setValue( "twitter/oauthtokensecret", oauthtokensecret );
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
    setValue( "xmppBot/port", -1 );
}

void 
TomahawkSettings::addScriptResolver(const QString& resolver)
{
    setValue( "script/resolvers", scriptResolvers() << resolver );
}

QStringList 
TomahawkSettings::scriptResolvers() const
{
    return value( "script/resolvers" ).toStringList();
}

void 
TomahawkSettings::setScriptResolvers( const QStringList& resolver )
{
    setValue( "script/resolvers", resolver );
}
