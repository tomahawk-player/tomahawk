/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

/**
 * Manages SIP plugins for connecting to friends. External interface to SIP plugins is
 * through AccountManager, this is an internal class.
 */

class SipHandler : public QObject
{
    Q_OBJECT

public:
    static SipHandler* instance();

    SipHandler( QObject* parent );
    ~SipHandler();

    void loadFromAccountManager();

#ifndef ENABLE_HEADLESS
    const QPixmap avatar( const QString& name ) const;
#endif

    //TODO: implement a proper SipInfo class and maybe attach it to the source
    const SipInfo sipInfo( const QString& peerId ) const;
    const QString versionString( const QString& peerId ) const;

    void hookUpPlugin( SipPlugin* p );

private slots:
    void onSipInfo( const QString& peerId, const SipInfo& info );
    void onSoftwareVersion( const QString& peerId, const QString& versionString );
    void onMessage( const QString&, const QString& );
    void onPeerOffline( const QString& );
    void onPeerOnline( const QString& );

#ifndef ENABLE_HEADLESS
    // set data for local source
    void onAvatarReceived( const QPixmap& avatar );

    // set data for other sources
    void onAvatarReceived( const QString& from, const QPixmap& avatar );
#endif

private:
    static SipHandler *s_instance;

    //TODO: move this to source
    QHash<QString, SipInfo> m_peersSipInfos;
    QHash<QString, QString> m_peersSoftwareVersions;
#ifndef ENABLE_HEADLESS
    QHash<QString, QPixmap> m_usernameAvatars;
#endif
};

#endif
