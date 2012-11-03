/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AVATARMANAGER_H
#define AVATARMANAGER_H

#include <jreen/client.h>

#include <QObject>
#include <QDir>

#include "accounts/AccountDllMacro.h"

class ACCOUNTDLLEXPORT AvatarManager : public QObject
{
Q_OBJECT

public:
    AvatarManager(Jreen::Client* client);
    virtual ~AvatarManager();

    QPixmap avatar(const QString& jid) const;

signals:
    void newAvatar( const QString& jid );

private slots:
    void onNewPresence( const Jreen::Presence& presence );
    void onNewIq(const Jreen::IQ& iq);
    void onNewConnection();
    void onNewAvatar( const QString& jid );

private:
    void fetchVCard( const QString& jid );
    QString avatarHash( const QString& jid ) const;
    QString avatarPath( const QString& avatarHash ) const;

    QDir avatarDir( const QString& avatarHash ) const;
    bool isCached( const QString& avatarHash ) const;

    Jreen::Client* m_client;
    QStringList m_cachedAvatars;
    QDir m_cacheDir;
    QMap< QString, QString > m_JidsAvatarHashes;
};

#endif // AVATARMANAGER_H
