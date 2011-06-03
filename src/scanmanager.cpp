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

#include "scanmanager.h"

#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QTimer>

#include "musicscanner.h"
#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"

#include "database/database.h"
#include "database/databasecommand_dirmtimes.h"

ScanManager* ScanManager::s_instance = 0;


ScanManager*
ScanManager::instance()
{
    return s_instance;
}


ScanManager::ScanManager( QObject* parent )
    : QObject( parent )
    , m_musicScannerThreadController( 0 )
    , m_currScannerPaths()
    , m_dirWatcher( 0 )
    , m_queuedScanTimer( 0 )
    , m_deferredScanTimer( 0 )
    , m_queuedChangedDirs()
    , m_deferredDirs()
{
    s_instance = this;

    m_queuedScanTimer = new QTimer( this );
    m_queuedScanTimer->setSingleShot( true );
    m_deferredScanTimer = new QTimer( this );
    m_deferredScanTimer->setSingleShot( false );
    m_deferredScanTimer->setInterval( 1000 );
    m_dirWatcher = new QFileSystemWatcher( this );

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );
    connect( m_queuedScanTimer, SIGNAL( timeout() ), SLOT( queuedScanTimeout() ) );
    connect( m_deferredScanTimer, SIGNAL( timeout() ), SLOT( deferredScanTimeout() ) );
    connect( m_dirWatcher, SIGNAL( directoryChanged( const QString & ) ), SLOT( handleChangedDir( const QString & ) ) );

    // FIXME: Disable this until we find something nondeprecated and working (e.g. not QFileSystemWatcher)
    TomahawkSettings::instance()->setWatchForChanges( false );
    
    if ( TomahawkSettings::instance()->hasScannerPaths() )
    {
        m_currScannerPaths = TomahawkSettings::instance()->scannerPaths();
        if ( TomahawkSettings::instance()->watchForChanges() )
            QTimer::singleShot( 1000, this, SLOT( runStartupScan() ) );
    }
    
    m_deferredScanTimer->start();
}


ScanManager::~ScanManager()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_scanner.isNull() )
    {
        m_musicScannerThreadController->quit();
        m_musicScannerThreadController->wait( 60000 );

        delete m_scanner.data();
        delete m_musicScannerThreadController;
        m_musicScannerThreadController = 0;
    }
}


void
ScanManager::onSettingsChanged()
{
    if ( TomahawkSettings::instance()->hasScannerPaths() &&
        m_currScannerPaths != TomahawkSettings::instance()->scannerPaths() )
    {
        m_currScannerPaths = TomahawkSettings::instance()->scannerPaths();
        m_dirWatcher->removePaths( m_dirWatcher->directories() );
        runManualScan( m_currScannerPaths );
    }

    if( TomahawkSettings::instance()->watchForChanges() &&
        !m_queuedChangedDirs.isEmpty() )
        runManualScan( m_queuedChangedDirs, false );
}


void ScanManager::runStartupScan()
{
    qDebug() << Q_FUNC_INFO;
    if( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        QTimer::singleShot( 1000, this, SLOT( runStartupScan() ) );
    else
        runManualScan( m_currScannerPaths );
}


void
ScanManager::runManualScan( const QStringList& paths, bool recursive )
{
    qDebug() << Q_FUNC_INFO;

    if( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        return;

    if ( !m_musicScannerThreadController && m_scanner.isNull() ) //still running if these are not zero
    {
        m_musicScannerThreadController = new QThread( this );
        QStringList allPaths = paths;
        foreach( QString path, m_deferredDirs[recursive] )
        {
            if( !allPaths.contains( path ) )
                allPaths << path;
        }
        m_scanner = QWeakPointer< MusicScanner>( new MusicScanner( paths, recursive ) );
        m_scanner.data()->moveToThread( m_musicScannerThreadController );
        connect( m_scanner.data(), SIGNAL( finished() ), SLOT( scannerFinished() ) );
        connect( m_scanner.data(), SIGNAL( addWatchedDirs( const QStringList & ) ), SLOT( addWatchedDirs( const QStringList & ) ) );
        connect( m_scanner.data(), SIGNAL( removeWatchedDir( const QString & ) ), SLOT( removeWatchedDir( const QString & ) ) );
        m_musicScannerThreadController->start( QThread::IdlePriority );
        QMetaObject::invokeMethod( m_scanner.data(), "startScan" );
        m_deferredDirs[recursive].clear();
    }
    else
    {
        qDebug() << "Could not run manual scan, old scan still running; deferring paths";
        foreach( QString path, paths )
        {
            if( !m_deferredDirs[recursive].contains( path ) )
            {
                qDebug() << "Deferring path " << path;
                m_deferredDirs[recursive] << path;
            }
        }
    }
}


void
ScanManager::addWatchedDirs( const QStringList& paths )
{
    qDebug() << Q_FUNC_INFO;
    if ( !TomahawkSettings::instance()->watchForChanges() )
        return;
    
    QStringList currentWatchedPaths = m_dirWatcher->directories();
    foreach ( QString path, paths )
    {
        if ( !currentWatchedPaths.contains( path ) )
        {
            qDebug() << "adding " << path << " to watched dirs";
            m_dirWatcher->addPath( path );
        }
    }
}


void
ScanManager::removeWatchedDir( const QString& path )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "removing " << path << " from watched dirs";
    m_dirWatcher->removePath( path );
}


void
ScanManager::handleChangedDir( const QString& path )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Dir changed: " << path;
    if( !m_queuedChangedDirs.contains( path ) )
        m_queuedChangedDirs << path;
    if( TomahawkSettings::instance()->watchForChanges() )
        m_queuedScanTimer->start( 10000 );
}


void
ScanManager::queuedScanTimeout()
{
    qDebug() << Q_FUNC_INFO;
    runManualScan( m_queuedChangedDirs, false );
    m_queuedChangedDirs.clear();
}


void
ScanManager::deferredScanTimeout()
{
    if( !m_deferredDirs[true].isEmpty() )
    {
        qDebug() << "Running scan for deferred recursive paths";
        runManualScan( m_deferredDirs[true], true );
    }
    else if( !m_deferredDirs[false].isEmpty() )
    {
        qDebug() << "Running scan for deferred non-recursive paths";
        runManualScan( m_deferredDirs[false], false );
    }
}


void
ScanManager::scannerFinished()
{
    if ( !m_scanner.isNull() )
    {
        m_musicScannerThreadController->quit();
        m_musicScannerThreadController->wait( 60000 );

        delete m_scanner.data();
        delete m_musicScannerThreadController;
        m_musicScannerThreadController = 0;
    }
    emit finished();
}
