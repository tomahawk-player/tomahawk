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
    , m_cacheBaseDir( QDesktopServices::storageLocation( QDesktopServices::CacheLocation ) + "/InfoSystemCache/" )
{
    qDebug() << Q_FUNC_INFO;

    m_pruneTimer.setInterval( 300000 );
    m_pruneTimer.setSingleShot( false );
    connect( &m_pruneTimer, SIGNAL( timeout() ), SLOT( pruneTimerFired() ) );
    m_pruneTimer.start();
}


InfoSystemCache::~InfoSystemCache()
{
    qDebug() << Q_FUNC_INFO;
}


void
InfoSystemCache::pruneTimerFired()
{
    qDebug() << Q_FUNC_INFO;

    qDebug() << "Pruning infosystemcache";
    qlonglong currentMSecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();
    
    for ( int i = 0; i <= InfoNoInfo; i++ )
    {
        InfoType type = (InfoType)(i);
        QHash< QString, QString > fileLocationHash = m_fileLocationCache[type];
        const QString cacheDirName = m_cacheBaseDir + QString::number( (int)type );
        QFileInfoList fileList = QDir( cacheDirName ).entryInfoList( QDir::Files | QDir::NoDotAndDotDot );
        foreach ( QFileInfo file, fileList )
        {
            QString baseName = file.baseName();
            if ( file.suffix().toLongLong() < currentMSecsSinceEpoch )
            {
                if ( !QFile::remove( file.canonicalFilePath() ) )
                    qDebug() << "Failed to remove stale cache file " << file.canonicalFilePath();
                else
                    qDebug() << "Removed stale cache file " << file.canonicalFilePath();
            }
            if ( fileLocationHash.contains( baseName ) )
                fileLocationHash.remove( baseName );
        }
        m_fileLocationCache[type] = fileLocationHash;
    }
}


void
InfoSystemCache::getCachedInfoSlot( const Tomahawk::InfoSystem::InfoCriteriaHash criteria, const qint64 newMaxAge, const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input, const Tomahawk::InfoSystem::InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;
    const QString criteriaHashVal = criteriaMd5( criteria );
    QHash< QString, QString > fileLocationHash = m_fileLocationCache[type];
    if ( !fileLocationHash.contains( criteriaHashVal ) )
    {
        if ( !fileLocationHash.isEmpty() )
        {
            //We already know of some values, so no need to re-read the directory again as it's already happened
            emit notInCache( criteria, caller, type, input, customData );
            return;
        }
            
        const QString cacheDir = m_cacheBaseDir + QString::number( (int)type );
        QDir dir( cacheDir );
        if ( !dir.exists() )
        {
            //Dir doesn't exist so clearly not in cache
            emit notInCache( criteria, caller, type, input, customData );
            return;
        }
        
        QFileInfoList fileList = dir.entryInfoList( QDir::Files | QDir::NoDotAndDotDot );
        foreach ( QFileInfo file, fileList )
        {
            QString baseName = file.baseName();
            fileLocationHash[baseName] = file.canonicalFilePath();
        }

        //Store what we've loaded up
        m_fileLocationCache[type] = fileLocationHash;
        if ( !fileLocationHash.contains( criteriaHashVal ) )
        {
            //Still didn't fine it? It's really not in the cache then
            emit notInCache( criteria, caller, type, input, customData );
            return;
        }
    }

    QFileInfo file( fileLocationHash[criteriaHashVal] );
    qlonglong currMaxAge = file.suffix().toLongLong();

    if ( currMaxAge < QDateTime::currentMSecsSinceEpoch() )
    {
        if ( !QFile::remove( file.canonicalFilePath() ) )
            qDebug() << "Failed to remove stale cache file " << file.canonicalFilePath();
        else
            qDebug() << "Removed stale cache file " << file.canonicalFilePath();
        
        fileLocationHash.remove( criteriaHashVal );
        m_fileLocationCache[type] = fileLocationHash;
        
        emit notInCache( criteria, caller, type, input, customData );
        return;
    }
    else if ( newMaxAge > 0 )
    {
        const QString newFilePath = QString( file.dir().canonicalPath() + '/' + criteriaHashVal + '.' + QString::number( QDateTime::currentMSecsSinceEpoch() + newMaxAge ) );
        
        if ( !QFile::rename( file.canonicalFilePath(), newFilePath ) )
        {
            qDebug() << "Failed to move old cache file to new location!";
            emit notInCache( criteria, caller, type, input, customData );
            return;
        }
        
        fileLocationHash[criteriaHashVal] = newFilePath;
        m_fileLocationCache[type] = fileLocationHash;
    }
    
    QSettings cachedSettings( fileLocationHash[criteriaHashVal], QSettings::IniFormat );
    QVariant output = cachedSettings.value( "data" );   

    emit info( caller, type, input, output, customData );
}


void
InfoSystemCache::updateCacheSlot( const Tomahawk::InfoSystem::InfoCriteriaHash criteria, const qint64 maxAge, const Tomahawk::InfoSystem::InfoType type, const QVariant output )
{
    qDebug() << Q_FUNC_INFO;
    
    const QString criteriaHashVal = criteriaMd5( criteria );
    const QString cacheDir = m_cacheBaseDir + QString::number( (int)type );
    const QString settingsFilePath( cacheDir + '/' + criteriaHashVal + '.' + QString::number( QDateTime::currentMSecsSinceEpoch() + maxAge ) );

    QHash< QString, QString > fileLocationHash = m_fileLocationCache[type];
    if ( fileLocationHash.contains( criteriaHashVal ) )
    {
        if ( !QFile::rename( fileLocationHash[criteriaHashVal], settingsFilePath ) )
        {
            qDebug() << "Failed to move old cache file to new location!";
            return;
        }
        fileLocationHash[criteriaHashVal] = settingsFilePath;
        m_fileLocationCache[type] = fileLocationHash;
        
        QSettings cachedSettings( fileLocationHash[criteriaHashVal], QSettings::IniFormat );
        cachedSettings.setValue( "data", output );
        
        return;
    }
    
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
    
    QSettings cachedSettings( settingsFilePath, QSettings::IniFormat );
    QStringList keys = criteria.keys();
    cachedSettings.beginGroup( "criteria" );
    for( int i = 0; i < criteria.size(); i++ )
        cachedSettings.setValue( keys.at( i ), criteria[keys.at( i )] );
    cachedSettings.endGroup();
    cachedSettings.setValue( "data", output );
    
    fileLocationHash[criteriaHashVal] = settingsFilePath;
    m_fileLocationCache[type] = fileLocationHash;
}


const QString
InfoSystemCache::criteriaMd5( const Tomahawk::InfoSystem::InfoCriteriaHash &criteria ) const
{
    QCryptographicHash hash( QCryptographicHash::Md5 );
    foreach( QString key, criteria.keys() )
        hash.addData( key.toUtf8() );
    foreach( QString value, criteria.values() )
        hash.addData( value.toUtf8() );
    return hash.result().toHex();
}


} //namespace InfoSystem

} //namespace Tomahawk
