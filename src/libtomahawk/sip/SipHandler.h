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

#include <QDebug>
#include <QObject>
#include <QHash>
#include <QPixmap>
#include <QString>

class SipHandler : public QObject
{
    Q_OBJECT

public:
    static SipHandler* instance();

    SipHandler( QObject* parent );
    ~SipHandler();

    QList< SipPlugin* > plugins() const;

    const QPixmap avatar( const QString& name ) const;

public slots:
    void addContact( const QString& id ) { qDebug() << Q_FUNC_INFO << id; }

    void checkSettings();
    void connectPlugins( bool startup = false, const QString &pluginName = QString() );
    void disconnectPlugins( const QString &pluginName = QString() );
    void toggleConnect();

signals:
    void connected();
    void disconnected();
    void authError();

private slots:
    void onMessage( const QString&, const QString& );
    void onPeerOffline( const QString& );
    void onPeerOnline( const QString& );
    void onError( int code, const QString& msg );

    void onSettingsChanged();

    void onAvatarReceived( const QString& from, const QPixmap& avatar = QPixmap());

private:
    static SipHandler *s_instance;

    QStringList findPlugins();
    bool pluginLoaded( const QString& name ) const;

    void loadPlugins( const QStringList& paths );
    void loadPlugin( const QString& path );

    QList< SipPlugin* > m_plugins;
    bool m_connected;


    QHash<QString, QPixmap> m_usernameAvatars;
};

#endif
