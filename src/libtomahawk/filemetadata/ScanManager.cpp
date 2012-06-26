/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "ScanManager.h"

#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include "MusicScanner.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtils.h"
#include "libtomahawk/SourceList.h"

#include "database/Database.h"
#include "database/DatabaseCommand_FileMTimes.h"
#include "database/DatabaseCommand_DeleteFiles.h"

#include "utils/Logger.h"

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
    , m_cachedScannerDirs()
{
    s_instance = this;

    m_scanTimer = new QTimer( this );
    m_scanTimer->setSingleShot( false );
    m_scanTimer->setInterval( TomahawkSettings::instance()->scannerTime() * 1000 );

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );
    connect( m_scanTimer, SIGNAL( timeout() ), SLOT( scanTimerTimeout() ) );

    if ( TomahawkSettings::instance()->hasScannerPaths() )
    {
        m_cachedScannerDirs = TomahawkSettings::instance()->scannerPaths();
        m_scanTimer->start();
        if ( TomahawkSettings::instance()->watchForChanges() )
            QTimer::singleShot( 1000, this, SLOT( runStartupScan() ) );
    }
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
    qDebug() << Q_FUNC_INFO << "scanner thread controller finished, exiting ScanManager";
}


void
ScanManager::onSettingsChanged()
{
    if ( !TomahawkSettings::instance()->watchForChanges() && m_scanTimer->isActive() )
        m_scanTimer->stop();

    m_scanTimer->setInterval( TomahawkSettings::instance()->scannerTime() * 1000 );

    if ( TomahawkSettings::instance()->hasScannerPaths() &&
        m_cachedScannerDirs != TomahawkSettings::instance()->scannerPaths() )
    {
        m_cachedScannerDirs = TomahawkSettings::instance()->scannerPaths();
        runNormalScan();
    }

    if ( TomahawkSettings::instance()->watchForChanges() && !m_scanTimer->isActive() )
        m_scanTimer->start();
}


void
ScanManager::runStartupScan()
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        QTimer::singleShot( 1000, this, SLOT( runStartupScan() ) );
    else
        runNormalScan();
}


void
ScanManager::scanTimerTimeout()
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( !TomahawkSettings::instance()->watchForChanges() ||
         !Database::instance() ||
         ( Database::instance() && !Database::instance()->isReady() ) )
        return;
    else
        runNormalScan();
}

void
ScanManager::runFullRescan()
{
    runNormalScan( true );
}



bool
ScanManager::runNormalScan( bool manualFull )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    if ( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        return false;

    if ( m_musicScannerThreadController || !m_scanner.isNull() ) //still running if these are not zero
    {
        tDebug( LOGVERBOSE ) << "Could not run dir scan, old scan still running";
        return false;
    }
    else
    {
        m_scanTimer->stop();
        m_musicScannerThreadController = new QThread( this );
        m_currScanMode = DirScan;
        
        if ( manualFull )
        {
            DatabaseCommand_DeleteFiles *cmd = new DatabaseCommand_DeleteFiles( SourceList::instance()->getLocal() );
            connect( cmd, SIGNAL( finished() ), SLOT( filesDeleted() ) );
            Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
            return true;
        }

        DatabaseCommand_FileMtimes *cmd = new DatabaseCommand_FileMtimes( true );
        connect( cmd, SIGNAL( done( const QMap< QString, QMap< unsigned int, unsigned int > >& ) ),
                        SLOT( fileMtimesCheck( const QMap< QString, QMap< unsigned int, unsigned int > >& ) ) );

        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
    }

    return true;
}


bool
ScanManager::runFileScan( const QStringList &paths )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    if ( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        return false;

    if ( m_musicScannerThreadController || !m_scanner.isNull() ) //still running if these are not zero
    {
        tDebug( LOGVERBOSE ) << "Could not run file scan, old scan still running";
        return false;
    }

    m_scanTimer->stop();
    m_musicScannerThreadController = new QThread( this );
    m_currScannerPaths = paths;
    m_currScanMode = FileScan;

    QMetaObject::invokeMethod( this, "runScan", Qt::QueuedConnection );

    return true;
}



void
ScanManager::fileMtimesCheck( const QMap< QString, QMap< unsigned int, unsigned int > >& mtimes )
{
    if ( !mtimes.isEmpty() && m_currScanMode == DirScan && TomahawkSettings::instance()->scannerPaths().isEmpty() )
    {
        DatabaseCommand_DeleteFiles *cmd = new DatabaseCommand_DeleteFiles( SourceList::instance()->getLocal() );
        connect( cmd, SIGNAL( finished() ), SLOT( filesDeleted() ) );
        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
        return;
    }

    QMetaObject::invokeMethod( this, "runScan", Qt::QueuedConnection );
}


void
ScanManager::filesDeleted()
{
    if ( !TomahawkSettings::instance()->scannerPaths().isEmpty() )
        QMetaObject::invokeMethod( this, "runScan", Qt::QueuedConnection );
    else
        scannerFinished();
}


void
ScanManager::runScan()
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO;

    QStringList paths = m_currScannerPaths.empty() ? TomahawkSettings::instance()->scannerPaths() : m_currScannerPaths;

    if ( m_musicScannerThreadController || !m_scanner.isNull() ) //still running if these are not zero
    {
        m_scanner = QWeakPointer< MusicScanner >( new MusicScanner( m_currScanMode, paths ) );
        m_scanner.data()->moveToThread( m_musicScannerThreadController );
        connect( m_scanner.data(), SIGNAL( finished() ), SLOT( scannerFinished() ) );
        m_musicScannerThreadController->start( QThread::IdlePriority );
        QMetaObject::invokeMethod( m_scanner.data(), "startScan" );
    }
    else
    {
        QTimer::singleShot( 200, this, SLOT( runScan() ) );
    }
}


void
ScanManager::scannerFinished()
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( m_musicScannerThreadController && !m_scanner.isNull() )
    {
        m_musicScannerThreadController->quit();
        m_musicScannerThreadController->wait( 60000 );

        delete m_scanner.data();
        delete m_musicScannerThreadController;
        m_musicScannerThreadController = 0;
    }

    m_scanTimer->start();
    m_currScannerPaths = QStringList();
    SourceList::instance()->getLocal()->scanningFinished( 0 );
    emit finished();
}
