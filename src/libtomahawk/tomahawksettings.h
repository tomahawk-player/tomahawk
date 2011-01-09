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
    
    /// General settings
    QString scannerPath() const; /// QDesktopServices::MusicLocation by default
    void setScannerPath( const QString& path );
    bool hasScannerPath() const;
    
    /// UI settings
    QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry( const QByteArray& geom );
    
    QByteArray mainWindowState() const;
    void setMainWindowState( const QByteArray& state );

    /// Playlist stuff
    QList<QVariant> playlistColumnSizes() const;
    void setPlaylistColumnSizes( const QList<QVariant>& cols );

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
    
    int jabberPort() const; // default is 5222
    void setJabberPort( int port );
    
    /// Network settings
    bool httpEnabled() const; /// true by default
    void setHttpEnabled( bool enable );
    
    bool upnpEnabled() const; /// true by default
    void setUPnPEnabled( bool enable );

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

    /// XMPP Component Settings
    QString xmppBotServer() const;
    void setXmppBotServer( const QString &server );

    QString xmppBotJid() const;
    void setXmppBotJid( const QString &component );

    QString xmppBotPassword() const;
    void setXmppBotPassword( const QString &password );

    int xmppBotPort() const;
    void setXmppBotPort( const int port );
    
private:
    static TomahawkSettings* s_instance;
};

#endif
