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

#include <QDebug>
#include <QObject>
#include <QHash>
#include <QPixmap>
#include <QString>

class DLLEXPORT SipHandler : public QObject
{
    Q_OBJECT

public:
    static SipHandler* instance();

    SipHandler( QObject* parent );
    ~SipHandler();

    QList< SipPluginFactory* > pluginFactories() const;
    QList< SipPlugin* > allPlugins() const;
    QList< SipPlugin* > enabledPlugins() const;
    QList< SipPlugin* > connectedPlugins() const;
    void loadFromConfig( bool startup = false );

    void addSipPlugin( SipPlugin* p, bool enable = true, bool connectImmediately = true );
    void removeSipPlugin( SipPlugin* p );

    bool hasPluginType( const QString& factoryId ) const;
    SipPluginFactory* factoryFromPlugin( SipPlugin* p ) const;

    const QPixmap avatar( const QString& name ) const;
    //TODO: implement a proper SipInfo class and maybe attach it to the source
    const SipInfo sipInfo( const QString& peerId ) const;

public slots:
    void checkSettings();

    void enablePlugin( SipPlugin* p );
    void disablePlugin( SipPlugin* p );

    void connectPlugin( bool startup = false, const QString &pluginId = QString() );
    void disconnectPlugin( const QString &pluginId = QString() );
    void connectAll();
    void disconnectAll();

    void toggleConnect();

    void setProxy( const QNetworkProxy &proxy );

    // create a new plugin of the given name. the name is the value returned in SipPluginFactory::pluginName
    // be default sip plugins are NOt connected when created
    SipPlugin* createPlugin( const QString& factoryName );
    // load a plugin with the given id
    SipPlugin* loadPlugin( const QString& pluginId );
    void removePlugin( SipPlugin* p );

signals:
    void connected( SipPlugin* );
    void disconnected( SipPlugin* );
    void authError( SipPlugin* );

    void stateChanged( SipPlugin* p, SipPlugin::ConnectionState state );

    void pluginAdded( SipPlugin* p );
    void pluginRemoved( SipPlugin* p );

private slots:
    void onSipInfo( const QString& peerId, const SipInfo& info );
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

    QHash< QString, SipPluginFactory* > m_pluginFactories;
    QList< SipPlugin* > m_allPlugins;
    QList< SipPlugin* > m_enabledPlugins;
    QList< SipPlugin* > m_connectedPlugins;
    bool m_connected;
    QNetworkProxy m_proxy;

    //TODO: move this to source
    QHash<QString, SipInfo> m_peersSipInfos;
    QHash<QString, QPixmap> m_usernameAvatars;
};

#endif
