#ifndef TOMAHAWK_SETTINGS_H
#define TOMAHAWK_SETTINGS_h

#include <QSettings>

/**
 * Convenience wrapper around QSettings for tomahawk-specific config
 */
class TomahawkSettings : public QSettings
{
    Q_OBJECT
public:
    explicit TomahawkSettings(QObject* parent = 0);
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
    
    QList<QVariant> playlistColumnSizes() const;
    void setPlaylistColumnSizes( const QList<QVariant>& cols );
    
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
    
    /// Last.fm settings
    bool scrobblingEnabled() const; /// false by default
    void setScrobblingEnabled( bool enable );
    
    QString lastFmUsername() const;
    void setLastFmUsername( const QString& username );
    
    QString lastFmPassword() const;
    void setLastFmPassword( const QString& password );
    
    QByteArray lastFmSessionKey() const;
    void setLastFmSessionKey( const QByteArray& key );
};

#endif
