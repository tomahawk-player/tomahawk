/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#ifndef SPOTIFYINFOPLUGIN_H
#define SPOTIFYINFOPLUGIN_H

#include "infosystem/InfoSystem.h"
#include "DllMacro.h"

#include <QWeakPointer>

class QNetworkReply;

namespace Tomahawk
{

namespace Accounts
{
    class SpotifyAccount;
}

namespace InfoSystem
{

class DLLEXPORT SpotifyInfoPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    explicit SpotifyInfoPlugin( Accounts::SpotifyAccount* account );
    virtual ~SpotifyInfoPlugin();

public slots:
    void  albumListingResult( const QString& msgType, const QVariantMap& msg, const QVariant& extraData );

protected slots:
    virtual void init() {}
    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );
    virtual void pushInfo( Tomahawk::InfoSystem::InfoPushData  );

private slots:
    void albumIdLookupFinished( QNetworkReply* reply, const Tomahawk::InfoSystem::InfoRequestData& requestData );
    void albumContentsLookupFinished( QNetworkReply* reply, const Tomahawk::InfoSystem::InfoRequestData& requestData );

private:
    void dataError( InfoRequestData );
    void trackListResult( const QStringList& trackNameList, const Tomahawk::InfoSystem::InfoRequestData& requestData );
    void sendLoveSong( const InfoType type, QVariant input );
    QWeakPointer< Tomahawk::Accounts::SpotifyAccount > m_account;
};

}

}

#endif // SPOTIFYINFOPLUGIN_H
