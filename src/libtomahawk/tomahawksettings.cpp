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
    , m_safety()
{
    s_instance = this;

    m_safety = new QMutex();
    QMutexLocker locker( m_safety );
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
    QMutexLocker locker( m_safety );
    #ifndef TOMAHAWK_HEADLESS
    return value( "scannerpath", QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) ).toString();
    #else
    return value( "scannerpath", "" ).toString();
    #endif
}


void
TomahawkSettings::setScannerPath( const QString& path )
{
    QMutexLocker locker( m_safety );
    setValue( "scannerpath", path );
}


bool
TomahawkSettings::hasScannerPath() const
{
    QMutexLocker locker( m_safety );
    return contains( "scannerpath" );
}


bool
TomahawkSettings::httpEnabled() const
{
    QMutexLocker locker( m_safety );
    return value( "network/http", true ).toBool();
}


void
TomahawkSettings::setHttpEnabled( bool enable )
{
    QMutexLocker locker( m_safety );
    setValue( "network/http", enable );
}


QString
TomahawkSettings::proxyHost() const
{
    QMutexLocker locker( m_safety );
    return value( "network/proxy/host", QString() ).toString();
}


void
TomahawkSettings::setProxyHost( const QString& host )
{
    QMutexLocker locker( m_safety );
    setValue( "network/proxy/host", host );
}


qulonglong
TomahawkSettings::proxyPort() const
{
    QMutexLocker locker( m_safety );
    return value( "network/proxy/port", 1080 ).toULongLong();
}


void
TomahawkSettings::setProxyPort( const qulonglong port )
{
    QMutexLocker locker( m_safety );
    setValue( "network/proxy/port", port );
}


QString
TomahawkSettings::proxyUsername() const
{
    QMutexLocker locker( m_safety );
    return value( "network/proxy/username", QString() ).toString();
}


void
TomahawkSettings::setProxyUsername( const QString& username )
{
    QMutexLocker locker( m_safety );
    setValue( "network/proxy/username", username );
}


QString
TomahawkSettings::proxyPassword() const
{
    QMutexLocker locker( m_safety );
    return value( "network/proxy/password", QString() ).toString();
}


void
TomahawkSettings::setProxyPassword( const QString& password )
{
    QMutexLocker locker( m_safety );
    setValue( "network/proxy/password", password );
}


int
TomahawkSettings::proxyType() const
{
    QMutexLocker locker( m_safety );
    return value( "network/proxy/type", 0 ).toInt();
}


void
TomahawkSettings::setProxyType( const int type )
{
    QMutexLocker locker( m_safety );
    setValue( "network/proxy/type", type );
}


QByteArray
TomahawkSettings::mainWindowGeometry() const
{
    QMutexLocker locker( m_safety );
    return value( "ui/mainwindow/geometry" ).toByteArray();
}


void
TomahawkSettings::setMainWindowGeometry( const QByteArray& geom )
{
    QMutexLocker locker( m_safety );
    setValue( "ui/mainwindow/geometry", geom );
}


QByteArray
TomahawkSettings::mainWindowState() const
{
    QMutexLocker locker( m_safety );
    return value( "ui/mainwindow/state" ).toByteArray();
}


void
TomahawkSettings::setMainWindowState( const QByteArray& state )
{
    QMutexLocker locker( m_safety );
    setValue( "ui/mainwindow/state", state );
}


QList<QVariant>
TomahawkSettings::playlistColumnSizes( const QString& playlistid ) const
{
    QMutexLocker locker( m_safety );
    return value( QString( "ui/playlist/%1/columnSizes" ).arg( playlistid ) ).toList();
}


void
TomahawkSettings::setPlaylistColumnSizes( const QString& playlistid, const QList<QVariant>& cols )
{
    QMutexLocker locker( m_safety );
    setValue( QString( "ui/playlist/%1/columnSizes" ).arg( playlistid ), cols );
}


QList<Tomahawk::playlist_ptr>
TomahawkSettings::recentlyPlayedPlaylists() const
{
    QMutexLocker locker( m_safety );
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
    QMutexLocker locker( m_safety );
    QStringList playlist_guids = value( "playlists/recentlyPlayed" ).toStringList();

    playlist_guids.removeAll( playlist->guid() );
    playlist_guids.append( playlist->guid() );

    setValue( "playlists/recentlyPlayed", playlist_guids );
}


bool
TomahawkSettings::jabberAutoConnect() const
{
    QMutexLocker locker( m_safety );
    return value( "jabber/autoconnect", true ).toBool();
}


void
TomahawkSettings::setJabberAutoConnect( bool autoconnect )
{
    QMutexLocker locker( m_safety );
    setValue( "jabber/autoconnect", autoconnect );
}


unsigned int
TomahawkSettings::jabberPort() const
{
    QMutexLocker locker( m_safety );
    return value( "jabber/port", 5222 ).toUInt();
}


void
TomahawkSettings::setJabberPort( int port )
{
    QMutexLocker locker( m_safety );
    if ( port < 0 )
      return;
    setValue( "jabber/port", port );
}


QString
TomahawkSettings::jabberServer() const
{
    QMutexLocker locker( m_safety );
    return value( "jabber/server" ).toString();
}


void
TomahawkSettings::setJabberServer( const QString& server )
{
    QMutexLocker locker( m_safety );
    setValue( "jabber/server", server );
}


QString
TomahawkSettings::jabberUsername() const
{
    QMutexLocker locker( m_safety );
    return value( "jabber/username" ).toString();
}


void
TomahawkSettings::setJabberUsername( const QString& username )
{
    QMutexLocker locker( m_safety );
    setValue( "jabber/username", username );
}


QString
TomahawkSettings::jabberPassword() const
{
    QMutexLocker locker( m_safety );
    return value( "jabber/password" ).toString();
}


void
TomahawkSettings::setJabberPassword( const QString& pw )
{
    QMutexLocker locker( m_safety );
    setValue( "jabber/password", pw );
}


TomahawkSettings::ExternalAddressMode
TomahawkSettings::externalAddressMode() const
{
    QMutexLocker locker( m_safety );
    return (TomahawkSettings::ExternalAddressMode) value( "network/external-address-mode", TomahawkSettings::Upnp ).toInt();
}


void
TomahawkSettings::setExternalAddressMode( ExternalAddressMode externalAddressMode )
{
    QMutexLocker locker( m_safety );
    setValue( "network/external-address-mode", externalAddressMode );
}

QString
TomahawkSettings::externalHostname() const
{
    QMutexLocker locker( m_safety );
    return value( "network/external-hostname" ).toString();
}

void
TomahawkSettings::setExternalHostname(const QString& externalHostname)
{
    QMutexLocker locker( m_safety );
    setValue( "network/external-hostname", externalHostname );
}

int
TomahawkSettings::externalPort() const
{
    QMutexLocker locker( m_safety );
    return value( "network/external-port" ).toInt();
}

void
TomahawkSettings::setExternalPort(int externalPort)
{
    QMutexLocker locker( m_safety );
    setValue( "network/external-port", externalPort);
}


QString
TomahawkSettings::lastFmPassword() const
{
    QMutexLocker locker( m_safety );
    return value( "lastfm/password" ).toString();
}


void
TomahawkSettings::setLastFmPassword( const QString& password )
{
    QMutexLocker locker( m_safety );
    setValue( "lastfm/password", password );
}


QByteArray
TomahawkSettings::lastFmSessionKey() const
{
    QMutexLocker locker( m_safety );
    return value( "lastfm/session" ).toByteArray();
}


void
TomahawkSettings::setLastFmSessionKey( const QByteArray& key )
{
    QMutexLocker locker( m_safety );
    setValue( "lastfm/session", key );
}


QString
TomahawkSettings::lastFmUsername() const
{
    QMutexLocker locker( m_safety );
    return value( "lastfm/username" ).toString();
}


void
TomahawkSettings::setLastFmUsername( const QString& username )
{
    QMutexLocker locker( m_safety );
    setValue( "lastfm/username", username );
}

QString
TomahawkSettings::twitterOAuthToken() const
{
    QMutexLocker locker( m_safety );
    return value( "twitter/OAuthToken" ).toString();
}

void
TomahawkSettings::setTwitterOAuthToken( const QString& oauthtoken )
{
    QMutexLocker locker( m_safety );
    setValue( "twitter/OAuthToken", oauthtoken );
}

QString
TomahawkSettings::twitterOAuthTokenSecret() const
{
    QMutexLocker locker( m_safety );
    return value( "twitter/OAuthTokenSecret" ).toString();
}

void
TomahawkSettings::setTwitterOAuthTokenSecret( const QString& oauthtokensecret )
{
    QMutexLocker locker( m_safety );
    setValue( "twitter/OAuthTokenSecret", oauthtokensecret );
}

qint64
TomahawkSettings::twitterCachedFriendsSinceId() const
{
    QMutexLocker locker( m_safety );
    return value( "twitter/CachedFriendsSinceID", 0 ).toLongLong();
}

void
TomahawkSettings::setTwitterCachedFriendsSinceId( qint64 cachedId )
{
    QMutexLocker locker( m_safety );
    setValue( "twitter/CachedFriendsSinceID", cachedId );
}

qint64
TomahawkSettings::twitterCachedMentionsSinceId() const
{
    QMutexLocker locker( m_safety );
    return value( "twitter/CachedMentionsSinceID", 0 ).toLongLong();
}

void
TomahawkSettings::setTwitterCachedMentionsSinceId( qint64 cachedId )
{
    QMutexLocker locker( m_safety );
    setValue( "twitter/CachedMentionsSinceID", cachedId );
}

qint64
TomahawkSettings::twitterCachedDirectMessagesSinceId() const
{
    QMutexLocker locker( m_safety );
    return value( "twitter/CachedDirectMessagesSinceID", 0 ).toLongLong();
}

void
TomahawkSettings::setTwitterCachedDirectMessagesSinceId( qint64 cachedId )
{
    QMutexLocker locker( m_safety );
    setValue( "twitter/CachedDirectMessagesSinceID", cachedId );
}

QHash<QString, QVariant>
TomahawkSettings::twitterCachedPeers() const
{
    QMutexLocker locker( m_safety );
    return value( "twitter/CachedPeers", QHash<QString, QVariant>() ).toHash();
}

void
TomahawkSettings::setTwitterCachedPeers( const QHash<QString, QVariant> &cachedPeers )
{
    QMutexLocker locker( m_safety );
    setValue( "twitter/CachedPeers", cachedPeers );
}

bool
TomahawkSettings::scrobblingEnabled() const
{
    QMutexLocker locker( m_safety );
    return value( "lastfm/enablescrobbling", false ).toBool();
}


void
TomahawkSettings::setScrobblingEnabled( bool enable )
{
    QMutexLocker locker( m_safety );
    setValue( "lastfm/enablescrobbling", enable );
}


QString
TomahawkSettings::xmppBotServer() const
{
    QMutexLocker locker( m_safety );
    return value( "xmppBot/server", QString() ).toString();
}


void
TomahawkSettings::setXmppBotServer( const QString& server )
{
    QMutexLocker locker( m_safety );
    setValue( "xmppBot/server", server );
}


QString
TomahawkSettings::xmppBotJid() const
{
    QMutexLocker locker( m_safety );
    return value( "xmppBot/jid", QString() ).toString();
}


void
TomahawkSettings::setXmppBotJid( const QString& component )
{
    QMutexLocker locker( m_safety );
    setValue( "xmppBot/jid", component );
}


QString
TomahawkSettings::xmppBotPassword() const
{
    QMutexLocker locker( m_safety );
    return value( "xmppBot/password", QString() ).toString();
}


void
TomahawkSettings::setXmppBotPassword( const QString& password )
{
    QMutexLocker locker( m_safety );
    setValue( "xmppBot/password", password );
}


int
TomahawkSettings::xmppBotPort() const
{
    QMutexLocker locker( m_safety );
    return value( "xmppBot/port", -1 ).toInt();
}


void
TomahawkSettings::setXmppBotPort( const int port )
{
    QMutexLocker locker( m_safety );
    setValue( "xmppBot/port", -1 );
}

void 
TomahawkSettings::addScriptResolver(const QString& resolver)
{
    QMutexLocker locker( m_safety );
    setValue( "script/resolvers", scriptResolvers() << resolver );
}

QStringList 
TomahawkSettings::scriptResolvers() const
{
    QMutexLocker locker( m_safety );
    return value( "script/resolvers" ).toStringList();
}

void 
TomahawkSettings::setScriptResolvers( const QStringList& resolver )
{
    QMutexLocker locker( m_safety );
    setValue( "script/resolvers", resolver );
}
