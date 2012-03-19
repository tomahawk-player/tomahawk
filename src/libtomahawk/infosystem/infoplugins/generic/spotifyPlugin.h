/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#ifndef SpotifyPlugin_H
#define SpotifyPlugin_H

#include "infosystem/infosystem.h"
#include "infosystem/infosystemworker.h"
#include <QNetworkReply>
#include <QObject>

class QNetworkReply;

namespace Tomahawk
{

namespace InfoSystem
{

class SpotifyPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    SpotifyPlugin();
    virtual ~SpotifyPlugin();

    enum ChartType {
        None =      0x00,
        Track =     0x01,
        Album =     0x02,
        Artist =    0x04

    };
 void setChartType( ChartType type ) { m_chartType = type; }
 ChartType chartType() const { return m_chartType; }

public slots:
    void chartReturned();
    void chartTypes();

protected slots:
    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );
    virtual void pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input )
    {
        Q_UNUSED( caller )
        Q_UNUSED( type)
        Q_UNUSED( input )
    }

private:
    void fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchChartCapabilities( Tomahawk::InfoSystem::InfoRequestData requestData );
    void dataError( Tomahawk::InfoSystem::InfoRequestData requestData );


    ChartType m_chartType;
    QVariantMap m_allChartsMap;
    uint m_chartsFetchJobs;
    QList< InfoRequestData > m_cachedRequests;
};

}

}

#endif // SpotifyPlugin_H
