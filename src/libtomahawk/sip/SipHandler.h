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

#ifndef SIPHANDLER_H
#define SIPHANDLER_H

#include "sip/SipPlugin.h"
#include "dllmacro.h"

#include <QObject>
#include <QHash>
#include <QString>

#ifndef ENABLE_HEADLESS
    #include <QPixmap>
#endif


class DLLEXPORT SipHandler : public QObject
{
    Q_OBJECT

public:
    static SipHandler* instance();

    SipHandler( QObject* parent );
    ~SipHandler();

    QList< SipPlugin* > allPlugins() const;
    QList< SipPlugin* > connectedPlugins() const;
    void loadFromAccountManager();

    void addSipPlugin( SipPlugin* p );
    void removeSipPlugin( SipPlugin* p );

    bool hasPluginType( const QString& factoryId ) const;

    const QPixmap avatar( const QString& name ) const;
    //TODO: implement a proper SipInfo class and maybe attach it to the source
    const SipInfo sipInfo( const QString& peerId ) const;
    const QString versionString( const QString& peerId ) const;

public slots:
    void checkSettings();

    void connectPlugin( const QString &pluginId = QString() );
    void disconnectPlugin( const QString &pluginId = QString() );
    void connectAll();
    void disconnectAll();

    void toggleConnect();

    void refreshProxy();

signals:
    void connected( SipPlugin* );
    void disconnected( SipPlugin* );
    void authError( SipPlugin* );

    void stateChanged( SipPlugin* p, SipPlugin::ConnectionState state );

    void pluginAdded( SipPlugin* p );
    void pluginRemoved( SipPlugin* p );

private slots:
    void onSipInfo( const QString& peerId, const SipInfo& info );
    void onSoftwareVersion( const QString& peerId, const QString& versionString );
    void onMessage( const QString&, const QString& );
    void onPeerOffline( const QString& );
    void onPeerOnline( const QString& );
    void onError( int code, const QString& msg );
    void onStateChanged( SipPlugin::ConnectionState );

    void onSettingsChanged();

    // set data for local source
    void onAvatarReceived( const QPixmap& avatar );

    // set data for other sources
    void onAvatarReceived( const QString& from, const QPixmap& avatar );

private:
    static SipHandler *s_instance;

    QStringList findPluginFactories();
    bool pluginLoaded( const QString& pluginId ) const;
    void hookUpPlugin( SipPlugin* p );

    void loadPluginFactories( const QStringList& paths );
    void loadPluginFactory( const QString& path );
    QString factoryFromId( const QString& pluginId ) const;

    QList< SipPlugin* > m_allPlugins;
    QList< SipPlugin* > m_enabledPlugins;
    QList< SipPlugin* > m_connectedPlugins;
    bool m_connected;

    //TODO: move this to source
    QHash<QString, SipInfo> m_peersSipInfos;
    QHash<QString, QPixmap> m_usernameAvatars;
    QHash<QString, QString> m_peersSoftwareVersions;
};

#endif
