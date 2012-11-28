/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2012, Casey Link <unnamedrambler@gmail.com>
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

#ifndef NEWRELEASESPLUGIN_H
#define NEWRELEASESPLUGIN_H

#include "infosystem/InfoSystem.h"
#include "infosystem/InfoSystemWorker.h"
#include "infoplugins/InfoPluginDllMacro.h"
#include <QVariantMap>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QObject>

class QNetworkReply;

namespace Tomahawk
{

namespace InfoSystem
{

class INFOPLUGINDLLEXPORT NewReleasesPlugin : public InfoPlugin
{
    Q_OBJECT
    Q_INTERFACES( Tomahawk::InfoSystem::InfoPlugin )
public:
    NewReleasesPlugin();
    virtual ~NewReleasesPlugin();

protected slots:
    virtual void init();
    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );

    virtual void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
    {
        Q_UNUSED( pushData );
    }

    /**
     * Parses a QNetworkReply of a list of newreleases sources.
     */
    void nrSourcesList();

    /**
     * Parses a QNetworkReply of a list of newreleases from a particular source
     */
    void nrList();

    /**
     * Parses a QNetworkReply for the newreleases data for a particular newrelease
     */
    void nrReturned();

private:
    /**
     * Requests newrelease list for each source in m_chartSources
     */
    void fetchAllNRSources();
    /**
     * Fetches a specific newrelease from a particular source.
     * Updates the cache.
     */
    void fetchNR( Tomahawk::InfoSystem::InfoRequestData requestData, const QString& source, const QString& nr_id );
    void fetchNRFromCache( Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchNRCapabilitiesFromCache( Tomahawk::InfoSystem::InfoRequestData requestData );
    void dataError( Tomahawk::InfoSystem::InfoRequestData requestData );

    qlonglong getMaxAge( const QByteArray &rawHeader ) const;
    qlonglong getMaxAge( const qlonglong expires ) const;

    QList< Tomahawk::InfoSystem::InfoStringHash > m_nrSources;
    QStringList m_refetchSource;
    QString m_nrVersion;
    QVariantMap m_allNRsMap;
    uint m_nrFetchJobs;
    QList< InfoRequestData > m_cachedRequests;
    QHash< QString, QString > m_cachedCountries;
    QWeakPointer< QNetworkAccessManager > m_nam;
};

}
}

#endif // NEWRELEASESPLUGIN_H
