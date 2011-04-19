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
#include <QCryptographicHash>

#include "infosystemcache.h"


namespace Tomahawk
{

namespace InfoSystem
{


InfoSystemCache::InfoSystemCache( QObject* parent )
    : QObject( parent )
    , m_syncTimer( this )
    , m_cacheRemainingToLoad( 0 )
{
    qDebug() << Q_FUNC_INFO;

    QString cacheBaseDir = QDesktopServices::storageLocation( QDesktopServices::CacheLocation );
    for( int i = 0; i <= InfoNoInfo; i++ )
    {
        InfoType type = (InfoType)(i);
        QString cacheDir = cacheBaseDir + "/InfoSystemCache/" + QString::number( i );
        QDir dir( cacheDir );
        if( dir.exists() )
        {
            m_cacheRemainingToLoad++;
            QMetaObject::invokeMethod( this, "loadCache", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QString, cacheDir ) );
        }
    }
    
    m_syncTimer.setInterval( 60000 );
    m_syncTimer.setSingleShot( false );
    connect( &m_syncTimer, SIGNAL( timeout() ), SLOT( syncTimerFired() ) );
    m_syncTimer.start();
}


InfoSystemCache::~InfoSystemCache()
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Saving infosystemcache to disk";
    QString cacheBaseDir = QDesktopServices::storageLocation( QDesktopServices::CacheLocation );
    for ( int i = 0; i <= InfoNoInfo; i++ )
    {
        InfoType type = (InfoType)(i);
        if ( m_dirtySet.contains( type ) && !m_dirtySet[type].isEmpty() && m_dataCache.contains( type ) )
        {
            QString cacheDir = cacheBaseDir + "/InfoSystemCache/" + QString::number( i );
            saveCache( type, cacheDir );
        }
    }
}


void
InfoSystemCache::syncTimerFired()
{
    qDebug() << Q_FUNC_INFO;
    if ( m_cacheRemainingToLoad > 0 )
        return;

    qDebug() << "Syncing infosystemcache to disk";
    QString cacheBaseDir = QDesktopServices::storageLocation( QDesktopServices::CacheLocation );
    for ( int i = 0; i <= InfoNoInfo; i++ )
    {
        InfoType type = (InfoType)(i);
        if ( m_dirtySet.contains( type ) && !m_dirtySet[type].isEmpty() && m_dataCache.contains( type ) )
        {
            QString cacheDir = cacheBaseDir + "/InfoSystemCache/" + QString::number( i );
            saveCache( type, cacheDir );
        }
    }
}


void
InfoSystemCache::getCachedInfoSlot( Tomahawk::InfoSystem::InfoCriteriaHash criteria, qint64 newMaxAge, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, Tomahawk::InfoSystem::InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !m_dataCache.contains( type ) || !m_dataCache[type].contains( criteria ) )
    {
        emit notInCache( criteria, caller, type, input, customData );
        return;
    }

    if ( m_cacheRemainingToLoad > 0 )
    {
        qDebug() << "Cache not fully loaded, punting request for a bit";
        QMetaObject::invokeMethod( this, "getCachedInfoSlot", Qt::QueuedConnection,  Q_ARG( Tomahawk::InfoSystem::InfoCriteriaHash, criteria ), Q_ARG( qint64, newMaxAge ), Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( Tomahawk::InfoSystem::InfoCustomData, customData ) );
        return;
    }
    
    QHash< InfoCriteriaHash, QDateTime > typemaxtimecache = m_maxTimeCache[type];
    
    if ( typemaxtimecache[criteria].toMSecsSinceEpoch() < QDateTime::currentMSecsSinceEpoch() )
    {
        QHash< InfoCriteriaHash, QVariant > typedatacache = m_dataCache[type];
        QHash< InfoCriteriaHash, QDateTime > typeinserttimecache = m_insertTimeCache[type];
        typemaxtimecache.remove( criteria );
        m_maxTimeCache[type] = typemaxtimecache;
        typedatacache.remove( criteria );
        m_dataCache[type] = typedatacache;
        typeinserttimecache.remove( criteria );
        m_insertTimeCache[type] = typeinserttimecache;
        emit notInCache( criteria, caller, type, input, customData );
        return;
    }
    
    if ( newMaxAge > 0 )
    {
        QHash< InfoCriteriaHash, QDateTime > typemaxtimecache = m_maxTimeCache[type];
        typemaxtimecache[criteria] = QDateTime::fromMSecsSinceEpoch( QDateTime::currentMSecsSinceEpoch() + newMaxAge );
        m_maxTimeCache[type] = typemaxtimecache;
        m_dirtySet[type].insert( criteria );
    }
    
    emit info( caller, type, input, m_dataCache[type][criteria], customData );
}


void
InfoSystemCache::updateCacheSlot( Tomahawk::InfoSystem::InfoCriteriaHash criteria, qint64 maxAge, Tomahawk::InfoSystem::InfoType type, QVariant output )
{
    qDebug() << Q_FUNC_INFO;
    QHash< InfoCriteriaHash, QVariant > typedatacache = m_dataCache[type];
    QHash< InfoCriteriaHash, QDateTime > typeinserttimecache = m_insertTimeCache[type];
    QHash< InfoCriteriaHash, QDateTime > typemaxtimecache = m_maxTimeCache[type];
    typedatacache[criteria] = output;
    typeinserttimecache[criteria] = QDateTime::currentDateTimeUtc();
    typemaxtimecache[criteria] = QDateTime::fromMSecsSinceEpoch( QDateTime::currentMSecsSinceEpoch() + maxAge );
    m_dataCache[type] = typedatacache;
    m_insertTimeCache[type] = typeinserttimecache;
    m_maxTimeCache[type] = typemaxtimecache;
    m_dirtySet[type].insert( criteria );
}


void
InfoSystemCache::loadCache( Tomahawk::InfoSystem::InfoType type, const QString &cacheDir )
{
    qDebug() << Q_FUNC_INFO;

    QDir dir( cacheDir );

    qDebug() << "Checking files in dir " << dir.canonicalPath();
    QFileInfoList files = dir.entryInfoList( QDir::Files );
    qDebug() << "Found " << files.size() << " files";

    QHash< InfoCriteriaHash, QVariant > dataHash = m_dataCache[type];
    QHash< InfoCriteriaHash, QDateTime > insertDateHash = m_insertTimeCache[type];
    QHash< InfoCriteriaHash, QDateTime > maxDateHash = m_maxTimeCache[type];
    QHash< InfoCriteriaHash, QString > fileLocationHash = m_fileLocationCache[type];
    
    foreach ( QFileInfo file, files )
    {
        if ( file.baseName().toLongLong() < QDateTime::currentMSecsSinceEpoch() )
        {
            if ( !QFile::remove( file.canonicalFilePath() ) )
                qDebug() << "Failed to remove stale cache file " << file.canonicalFilePath();
            else
                qDebug() << "Removed stale cache file " << file.canonicalFilePath();
            continue;
        }
            
        QSettings cachedSettings( file.canonicalFilePath(), QSettings::IniFormat );


        InfoCriteriaHash criteria;
        cachedSettings.beginGroup( "criteria" );
        foreach ( QString key, cachedSettings.childKeys() )
            criteria[key] = cachedSettings.value( key ).toString();
        cachedSettings.endGroup();
        dataHash[criteria] = cachedSettings.value( "data" );
        insertDateHash[criteria] = cachedSettings.value( "inserttime" ).toDateTime();
        maxDateHash[criteria] =  QDateTime::fromMSecsSinceEpoch( file.baseName().toLongLong() );
        fileLocationHash[criteria] = file.canonicalFilePath();
    }

    m_dataCache[type] = dataHash;
    m_insertTimeCache[type] = insertDateHash;
    m_maxTimeCache[type] = maxDateHash;
    m_fileLocationCache[type] = fileLocationHash;

    m_cacheRemainingToLoad--;
}


void
InfoSystemCache::saveCache( Tomahawk::InfoSystem::InfoType type, const QString &cacheDir )
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

    QHash< InfoCriteriaHash, QString > fileLocationHash = m_fileLocationCache[type];

    foreach ( InfoCriteriaHash criteria, m_dirtySet[type].values() )
    {
        QString maxAge( QString::number( m_maxTimeCache[type][criteria].toMSecsSinceEpoch() ) );
        QCryptographicHash hash( QCryptographicHash::Md5 );
        foreach( QString key, criteria.keys() )
            hash.addData( key.toUtf8() );
        foreach( QString value, criteria.values() )
            hash.addData( value.toUtf8() );
        QString settingsFilePath( cacheDir + '/' + maxAge + '.' + hash.result().toHex() );
        if ( m_fileLocationCache[type].contains( criteria ) )
        {
            if ( !QFile::rename( m_fileLocationCache[type][criteria], settingsFilePath ) )
                qDebug() << "Failed to move old cache file to new location!";
            else
                m_dirtySet[type].remove( criteria );
            continue;
        }
        QSettings cachedSettings( settingsFilePath, QSettings::IniFormat );
        QStringList keys = criteria.keys();
        cachedSettings.beginGroup( "criteria" );
        for( int i = 0; i < criteria.size(); i++ )
            cachedSettings.setValue( keys.at( i ), criteria[keys.at( i )] );
        cachedSettings.endGroup();
        cachedSettings.setValue( "data", m_dataCache[type][criteria] );
        cachedSettings.setValue( "inserttime", m_insertTimeCache[type][criteria] );
        fileLocationHash[criteria] = settingsFilePath;
        m_dirtySet[type].remove( criteria );
    }

    m_fileLocationCache[type] = fileLocationHash;

}


} //namespace InfoSystem

} //namespace Tomahawk
