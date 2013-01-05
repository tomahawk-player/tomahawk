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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TOMAHAWK_INFOSYSTEMCACHE_H
#define TOMAHAWK_INFOSYSTEMCACHE_H

#include <QCache>
#include <QDateTime>
#include <QObject>
#include <QtDebug>
#include <QTimer>

#include "InfoSystem.h"

namespace Tomahawk
{

namespace InfoSystem
{

class InfoSystemCache : public QObject
{
Q_OBJECT

public:
    InfoSystemCache( QObject *parent = 0 );

    virtual ~InfoSystemCache();

signals:
    void info( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );

public slots:
    void getCachedInfoSlot( Tomahawk::InfoSystem::InfoStringHash criteria, qint64 newMaxAge, Tomahawk::InfoSystem::InfoRequestData requestData );
    void updateCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, qint64 maxAge, Tomahawk::InfoSystem::InfoType type, QVariant output );

private slots:
    void pruneTimerFired();

private:
    void notInCache( QObject *receiver, Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );
    void doUpgrade( uint oldVersion, uint newVersion );
    void performWipe( QString directory );
    const QString criteriaMd5( const Tomahawk::InfoSystem::InfoStringHash &criteria, Tomahawk::InfoSystem::InfoType type = Tomahawk::InfoSystem::InfoNoInfo ) const;

    QString m_cacheBaseDir;
    QHash< InfoType, QHash< QString, QString > > m_fileLocationCache;
    QTimer m_pruneTimer;
    QCache< QString, QVariant > m_dataCache;

    uint m_cacheVersion;
};

} //namespace InfoSystem

} //namespace Tomahawk

Q_DECLARE_METATYPE( Tomahawk::InfoSystem::InfoSystemCache* );

#endif //TOMAHAWK_INFOSYSTEMCACHE_H
