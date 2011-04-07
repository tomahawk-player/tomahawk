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

#include <QtDebug>
#include <QDesktopServices>
#include <QDir>
#include <QSettings>

#include "infosystemcache.h"


namespace Tomahawk
{

namespace InfoSystem
{


InfoSystemCache::InfoSystemCache( QObject* parent )
    : QObject(parent)
{
    qDebug() << Q_FUNC_INFO;
    QString cacheBaseDir = QDesktopServices::storageLocation( QDesktopServices::CacheLocation );
    for( int i = 0; i <= InfoNoInfo; i++ )
    {
        InfoType type = (InfoType)(i);
        QString cacheDir = cacheBaseDir + "/InfoSystemCache/" + QString::number( i );
        QString cacheFile = cacheDir + '/' + QString::number( i );
        QDir dir( cacheDir );
        if( dir.exists() && QFile::exists( cacheFile ) )
            loadCache( type, cacheFile );
    }
}


InfoSystemCache::~InfoSystemCache()
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Saving infosystemcache to disk";
    QString cacheBaseDir = QDesktopServices::storageLocation( QDesktopServices::CacheLocation );
    for ( int i = 0; i <= InfoNoInfo; i++ )
    {
        InfoType type = (InfoType)(i);
        if ( m_dirtySet.contains( type ) && m_dataCache.contains( type ) )
        {
            QString cacheDir = cacheBaseDir + "/InfoSystemCache/" + QString::number( i );
            saveCache( type, cacheDir );
        }
    }
}


void
InfoSystemCache::getCachedInfoSlot( Tomahawk::InfoSystem::InfoCacheCriteria criteria, qint64 newMaxAge, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, Tomahawk::InfoSystem::InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !m_dataCache.contains( type ) || !m_dataCache[type].contains( criteria ) )
    {
        emit notInCache( criteria, caller, type, input, customData );
        return;
    }
    
    QHash< InfoCacheCriteria, QDateTime > typemaxtimecache = m_maxTimeCache[type];
    
    if ( typemaxtimecache[criteria].toMSecsSinceEpoch() < QDateTime::currentMSecsSinceEpoch() )
    {
        QHash< InfoCacheCriteria, QVariant > typedatacache = m_dataCache[type];
        QHash< InfoCacheCriteria, QDateTime > typeinserttimecache = m_insertTimeCache[type];
        typemaxtimecache.remove( criteria );
        m_maxTimeCache[type] = typemaxtimecache;
        typedatacache.remove( criteria );
        m_dataCache[type] = typedatacache;
        typeinserttimecache.remove( criteria );
        m_insertTimeCache[type] = typeinserttimecache;
        m_dirtySet.insert( type );
        emit notInCache( criteria, caller, type, input, customData );
        return;
    }
    
    if ( newMaxAge > 0 )
    {
        QHash< InfoCacheCriteria, QDateTime > typemaxtimecache = m_maxTimeCache[type];
        typemaxtimecache[criteria] = QDateTime::fromMSecsSinceEpoch( QDateTime::currentMSecsSinceEpoch() + newMaxAge );
        m_maxTimeCache[type] = typemaxtimecache;
        m_dirtySet.insert( type );
    }
    
    emit info( caller, type, input, m_dataCache[type][criteria], customData );
}


void
InfoSystemCache::updateCacheSlot( Tomahawk::InfoSystem::InfoCacheCriteria criteria, qint64 maxAge, Tomahawk::InfoSystem::InfoType type, QVariant output )
{
    qDebug() << Q_FUNC_INFO;
    QHash< InfoCacheCriteria, QVariant > typedatacache = m_dataCache[type];
    QHash< InfoCacheCriteria, QDateTime > typeinserttimecache = m_insertTimeCache[type];
    QHash< InfoCacheCriteria, QDateTime > typemaxtimecache = m_maxTimeCache[type];
    typedatacache[criteria] = output;
    typeinserttimecache[criteria] = QDateTime::currentDateTimeUtc();
    typemaxtimecache[criteria] = QDateTime::fromMSecsSinceEpoch( QDateTime::currentMSecsSinceEpoch() + maxAge );
    m_dataCache[type] = typedatacache;
    m_insertTimeCache[type] = typeinserttimecache;
    m_maxTimeCache[type] = typemaxtimecache;
    m_dirtySet.insert( type );
}


void
InfoSystemCache::loadCache( InfoType type, const QString &cacheFile )
{
    qDebug() << Q_FUNC_INFO;
    QSettings cachedSettings( cacheFile, QSettings::IniFormat );

    foreach ( QString group, cachedSettings.childGroups() )
    {
        cachedSettings.beginGroup( group );
        if ( cachedSettings.value( "maxtime" ).toDateTime().toMSecsSinceEpoch() < QDateTime::currentMSecsSinceEpoch() )
            continue;
        QHash< InfoCacheCriteria, QVariant > dataHash = m_dataCache[type];
        QHash< InfoCacheCriteria, QDateTime > insertDateHash = m_insertTimeCache[type];
        QHash< InfoCacheCriteria, QDateTime > maxDateHash = m_maxTimeCache[type];
        InfoCacheCriteria criteria;
        int numCriteria = cachedSettings.beginReadArray( "criteria" );
        for ( int i = 0; i < numCriteria; i++ )
        {
            cachedSettings.setArrayIndex( i );
            QStringList criteriaValues = cachedSettings.value( QString::number( i ) ).toStringList();
            for ( int j = 0; j < criteriaValues.length(); j += 2 )
                criteria[criteriaValues.at( j )] = criteriaValues.at( j + 1 );
        }
        cachedSettings.endArray();
        dataHash[criteria] = cachedSettings.value( "data" );
        insertDateHash[criteria] = cachedSettings.value( "inserttime" ).toDateTime();
        maxDateHash[criteria] = cachedSettings.value( "maxtime" ).toDateTime();
        cachedSettings.endGroup();
        m_dataCache[type] = dataHash;
        m_insertTimeCache[type] = insertDateHash;
        m_maxTimeCache[type] = maxDateHash;
    }
}


void
InfoSystemCache::saveCache( InfoType type, const QString &cacheDir )
{
    qDebug() << Q_FUNC_INFO;
    QDir dir( cacheDir );
    if( !dir.exists( cacheDir ) )
    {
        qDebug() << "Creating cache directory " << cacheDir;
        if( !dir.mkpath( cacheDir ) )
        {
            qDebug() << "Failed to create cache dir! Bailing...";
            return;
        }
    }

    QSettings cachedSettings( QString( cacheDir + '/' + QString::number( (int)type ) ), QSettings::IniFormat );

    int criteriaNumber = 0;

    foreach( InfoCacheCriteria criteria, m_dataCache[type].keys() )
    {
        cachedSettings.beginGroup( "group_" +  QString::number( criteriaNumber ) );
        cachedSettings.beginWriteArray( "criteria" );
        QStringList keys = criteria.keys();
        for( int i = 0; i < criteria.size(); i++ )
        {
            cachedSettings.setArrayIndex( i );
            QStringList critVal;
            critVal << keys.at( i ) << criteria[keys.at( i )];
            cachedSettings.setValue( QString::number( i ), critVal );
        }
        cachedSettings.endArray();
        cachedSettings.setValue( "data", m_dataCache[type][criteria] );
        cachedSettings.setValue( "inserttime", m_insertTimeCache[type][criteria] );
        cachedSettings.setValue( "maxtime", m_maxTimeCache[type][criteria] );
        cachedSettings.endGroup();
        ++criteriaNumber;
    }

    m_dirtySet.remove( type );
}


} //namespace InfoSystem

} //namespace Tomahawk