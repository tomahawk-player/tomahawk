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
        if( m_dirtySet.contains( type ) && m_dataCache.contains( type ) )
        {
            QString cacheDir = cacheBaseDir + QString::number( i );
            QDir dir( cacheDir );
            if( dir.exists() && QFile::exists( QString( cacheDir + '/' + QString::number( i ) ) ) )
                loadCache( type, QString( cacheDir + '/' + QString::number( i ) ) );
        }
    }
}


InfoSystemCache::~InfoSystemCache()
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Saving infosystemcache to disk";
    QString cacheBaseDir = QDesktopServices::storageLocation( QDesktopServices::CacheLocation );
    for( int i = 0; i <= InfoNoInfo; i++ )
    {
        InfoType type = (InfoType)(i);
        if( m_dirtySet.contains( type ) && m_dataCache.contains( type ) )
        {
            QString cacheDir = cacheBaseDir + QString::number( i );
            saveCache( type, cacheDir );
        }
    }
}


void
InfoSystemCache::getCachedInfoSlot( Tomahawk::InfoSystem::InfoCacheCriteria criteria, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, Tomahawk::InfoSystem::InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;
    if( !m_dataCache.contains( type ) || !m_dataCache[type].contains( criteria ) )
    {
        emit notInCache( criteria, caller, type, input, customData );
        return;
    }
    
    emit info( caller, type, input, m_dataCache[type][criteria], customData );
}


void
InfoSystemCache::updateCacheSlot( Tomahawk::InfoSystem::InfoCacheCriteria criteria, Tomahawk::InfoSystem::InfoType type, QVariant output )
{
    qDebug() << Q_FUNC_INFO;
    QHash< InfoCacheCriteria, QVariant > typedatacache;
    QHash< InfoCacheCriteria, QDateTime > typetimecache;
    typedatacache[criteria] = output;
    typetimecache[criteria] = QDateTime::currentDateTimeUtc();
    m_dataCache[type] = typedatacache;
    m_timeCache[type] = typetimecache;
    m_dirtySet.insert( type );
}


void
InfoSystemCache::loadCache( InfoType type, const QString &cacheDir )
{
    qDebug() << Q_FUNC_INFO;
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

    QSettings cacheFile( QString( cacheDir + '/' + QString::number( (int)type ) ), QSettings::IniFormat );
    
    foreach( InfoCacheCriteria criteria, m_dataCache[type].keys() )
    {
        cacheFile.beginGroup( "type_" +  QString::number( type ) );
        cacheFile.beginWriteArray( "criteria" );
        QStringList keys = criteria.keys();
        for( int i = 0; i < criteria.size(); i++ )
        {
            cacheFile.setArrayIndex( i );
            cacheFile.setValue( keys.at( i ), criteria[keys.at( i )] );
        }
        cacheFile.endArray();
        cacheFile.setValue( "data", m_dataCache[type][criteria] );
        cacheFile.setValue( "time", m_timeCache[type][criteria] );
        cacheFile.endGroup();
    }

    m_dirtySet.remove( type );
}


} //namespace InfoSystem

} //namespace Tomahawk