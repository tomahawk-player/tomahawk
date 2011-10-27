/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef ROVIPLUGIN_H
#define ROVIPLUGIN_H

#include "infosystem/infosystem.h"

#include <QNetworkReply>

class QNetworkAccessManager;

namespace Tomahawk
{

namespace InfoSystem
{

class RoviPlugin : public InfoPlugin
{
    Q_OBJECT
public:
    RoviPlugin();
    virtual ~RoviPlugin();

protected:
    virtual void namChangedSlot( QNetworkAccessManager* nam );
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );

    virtual void pushInfo( QString, Tomahawk::InfoSystem::InfoType, QVariant )
    {}

    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );

private slots:
    void albumLookupFinished();
    void albumLookupError( QNetworkReply::NetworkError );
private:
    QNetworkReply* makeRequest( QUrl url );
    QByteArray generateSig() const;

    QNetworkAccessManager* m_nam;

    QByteArray m_apiKey;
    QByteArray m_secret;
};

}

}

#endif // ROVIPLUGIN_H
