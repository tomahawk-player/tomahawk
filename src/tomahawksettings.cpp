#include "tomahawk/tomahawkapp.h"
#include "tomahawksettings.h"

#ifndef TOMAHAWK_HEADLESS
    #include <QDesktopServices>
    #include "settingsdialog.h"
#endif

#include <QDir>
#include <QDebug>


TomahawkSettings::TomahawkSettings( QObject* parent )
    : QSettings( parent )
{
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
}


QString TomahawkSettings::scannerPath() const
{
    #ifndef TOMAHAWK_HEADLESS
    return value( "scannerpath", QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) ).toString();
    #else
    return value( "scannerpath", "" ).toString();
    #endif
}


void TomahawkSettings::setScannerPath(const QString& path)
{
    setValue( "scannerpath", path );
}


bool TomahawkSettings::hasScannerPath() const
{
    return contains( "scannerpath" );
}


bool TomahawkSettings::httpEnabled() const
{
    return value( "network/http", true ).toBool();
}


void TomahawkSettings::setHttpEnabled(bool enable)
{
    setValue( "network/http", enable );
}


QByteArray TomahawkSettings::mainWindowGeometry() const
{
    return value( "ui/mainwindow/geometry" ).toByteArray();
}


void TomahawkSettings::setMainWindowGeometry(const QByteArray& geom)
{
    setValue( "ui/mainwindow/geometry", geom );
}


QByteArray TomahawkSettings::mainWindowState() const
{
    return value( "ui/mainwindow/state" ).toByteArray();
}


void TomahawkSettings::setMainWindowState(const QByteArray& state)
{
    setValue( "ui/mainwindow/state", state );
}


QList<QVariant> TomahawkSettings::playlistColumnSizes() const
{
    return value( "ui/playlist/columnSize" ).toList();
}


void TomahawkSettings::setPlaylistColumnSizes(const QList<QVariant>& cols)
{
    setValue( "ui/playlist/geometry", cols );
}


bool TomahawkSettings::jabberAutoConnect() const
{
    return value( "jabber/autoconnect", true ).toBool();
}


void TomahawkSettings::setJabberAutoConnect(bool autoconnect)
{
    setValue( "jabber/autoconnect", autoconnect );
}


int TomahawkSettings::jabberPort() const
{
    return value( "jabber/port", 5222 ).toInt();
}


void TomahawkSettings::setJabberPort(int port)
{
    setValue( "jabber/port", port );
}


QString TomahawkSettings::jabberServer() const
{
    return value( "jabber/server" ).toString();
}


void TomahawkSettings::setJabberServer(const QString& server)
{
    setValue( "jabber/server", server );
}


QString TomahawkSettings::jabberUsername() const
{
    return value( "jabber/username" ).toString();
}


void TomahawkSettings::setJabberUsername(const QString& username)
{
    setValue( "jabber/username", username );
}


QString TomahawkSettings::jabberPassword() const
{
    return value( "jabber/password" ).toString();
}


void TomahawkSettings::setJabberPassword(const QString& pw)
{
    setValue( "jabber/password", pw );
}


bool TomahawkSettings::upnpEnabled() const
{
    return value( "network/upnp", true ).toBool();
}


void TomahawkSettings::setUPnPEnabled(bool enable)
{
    setValue( "network/upnp", enable );
}


QString TomahawkSettings::lastFmPassword() const
{
    return value( "lastfm/password" ).toString();
}


void TomahawkSettings::setLastFmPassword(const QString& password)
{
    setValue( "lastfm/password", password );
}


QByteArray TomahawkSettings::lastFmSessionKey() const
{
    return value( "lastfm/sessionkey" ).toByteArray();
}


void TomahawkSettings::setLastFmSessionKey(const QByteArray& key)
{
    setValue( "lastfm/sessionkey", key );
}


QString TomahawkSettings::lastFmUsername() const
{
    return value( "lastfm/username" ).toString();
}


void TomahawkSettings::setLastFmUsername(const QString& username )
{
    setValue( "lastfm/username", username );
}


bool TomahawkSettings::scrobblingEnabled() const
{
    return value( "lastfm/enablescrobbling", false ).toBool();
}


void TomahawkSettings::setScrobblingEnabled(bool enable)
{
    setValue( "lastfm/enablescrobbling", enable );
}
