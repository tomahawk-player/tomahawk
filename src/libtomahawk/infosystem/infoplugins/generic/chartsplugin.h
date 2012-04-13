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

#ifndef ChartsPlugin_H
#define ChartsPlugin_H

#include "infosystem/infosystem.h"
#include "infosystem/infosystemworker.h"
#include <QtNetwork/QNetworkReply>
#include <QtCore/QObject>

class QNetworkReply;

namespace Tomahawk
{

namespace InfoSystem
{

class ChartsPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    ChartsPlugin();
    virtual ~ChartsPlugin();

    enum ChartType {
        None =      0x00,
        Track =     0x01,
        Album =     0x02,
        Artist =    0x04

    };

    void setChartType( ChartType type ) { m_chartType = type; }
    ChartType chartType() const { return m_chartType; }

protected slots:
    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );

    virtual void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
    {
        Q_UNUSED( pushData );
    }

    /**
     * Parses a QNetworkReply of a list of chart sources.
     */
    void chartSourcesList();

    /**
     * Parses a QNetworkReply of a list of charts for a particular source
     */
    void chartsList();

    /**
     * Parses a QNetworkReply for the chart data for a particular chart
     */
    void chartReturned();

private:
    /**
     * Fetch list of chart sources (e.g., itunes, billboard)
     * Populates the m_chartResources member.
     */
    void fetchChartSourcesList( bool fetchOnlySourceList );
    /**
     * Requests charts list for each chart source in m_chartResources
     */
    void fetchAllChartSources();
    /**
     * Fetches a specific chart from a particular source.
     * Updates the cache.
     */
    void fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData, const QString& source, const QString& chart_id );

    void fetchChartFromCache( Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchChartCapabilitiesFromCache( Tomahawk::InfoSystem::InfoRequestData requestData );
    void dataError( Tomahawk::InfoSystem::InfoRequestData requestData );

    QStringList m_chartResources;
    QString m_chartVersion;
    QList< InfoStringHash > m_charts;
    ChartType m_chartType;
    QVariantMap m_allChartsMap;
    uint m_chartsFetchJobs;
    QList< InfoRequestData > m_cachedRequests;
    QHash< QString, QString > m_cachedCountries;
    QWeakPointer< QNetworkAccessManager > m_nam;
};

}

}

#endif // ChartsPlugin_H
