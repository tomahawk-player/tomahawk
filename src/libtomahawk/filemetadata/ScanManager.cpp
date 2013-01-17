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

#include "MusicScanner.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtils.h"
#include "SourceList.h"

#include "database/Database.h"
#include "database/DatabaseCommand_FileMTimes.h"
#include "database/DatabaseCommand_DeleteFiles.h"

#include "utils/Logger.h"

#include <QThread>
#include <QCoreApplication>
#include <QTimer>
#include <QSet>


MusicScannerThreadController::MusicScannerThreadController( QObject* parent )
    : QThread( parent )
    , m_bs( 0 )
{
    tDebug() << Q_FUNC_INFO;
}


MusicScannerThreadController::~MusicScannerThreadController()
{
   tDebug() << Q_FUNC_INFO;
}


void
MusicScannerThreadController::run()
{
    m_musicScanner = QPointer< MusicScanner >( new MusicScanner( m_mode, m_paths, m_bs ) );
    connect( m_musicScanner.data(), SIGNAL( finished() ), parent(), SLOT( scannerFinished() ), Qt::QueuedConnection );
    QMetaObject::invokeMethod( m_musicScanner.data(), "startScan", Qt::QueuedConnection );

    exec();

    if ( !m_musicScanner.isNull() )
      delete m_musicScanner.data();
}


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
    , m_queuedScanType( MusicScanner::None )
    , m_updateGUI( true )
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

    if ( m_musicScannerThreadController )
    {
        m_musicScannerThreadController->quit();
        m_musicScannerThreadController->wait( 60000 );

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


void
ScanManager::runNormalScan( bool manualFull )
{
    if ( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
    {
        tLog() << Q_FUNC_INFO << "Error... Database is not ready, but should be";
        return;
    }

    if ( QThread::currentThread() != ScanManager::instance()->thread() )
    {
        QMetaObject::invokeMethod( this, "runNormalScan", Qt::QueuedConnection, Q_ARG( bool, manualFull ) );
        return;
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    if ( m_musicScannerThreadController ) //still running if these are not zero
    {
        if ( m_queuedScanType != MusicScanner::Full )
            m_queuedScanType = manualFull ? MusicScanner::Full : MusicScanner::Normal;
        tDebug( LOGVERBOSE ) << "Could not run dir scan, old scan still running";
        return;
    }

    m_scanTimer->stop();
    m_musicScannerThreadController = new MusicScannerThreadController( this );
    m_currScanMode = MusicScanner::DirScan;

    if ( manualFull )
    {
        DatabaseCommand_DeleteFiles *cmd = new DatabaseCommand_DeleteFiles( SourceList::instance()->getLocal() );
        connect( cmd, SIGNAL( finished() ), SLOT( filesDeleted() ) );
        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
        return;
    }

    DatabaseCommand_FileMtimes *cmd = new DatabaseCommand_FileMtimes( true );
    connect( cmd, SIGNAL( done( const QMap< QString, QMap< unsigned int, unsigned int > >& ) ),
                    SLOT( fileMtimesCheck( const QMap< QString, QMap< unsigned int, unsigned int > >& ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
}


void
ScanManager::runFileScan( const QStringList& paths, bool updateGUI )
{
    if ( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
    {
        tLog() << Q_FUNC_INFO << "Error... Database is not ready, but should be";
        return;
    }

    if ( QThread::currentThread() != ScanManager::instance()->thread() )
    {
        QMetaObject::invokeMethod( this, "runFileScan", Qt::QueuedConnection, Q_ARG( QStringList, paths ) );
        return;
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    foreach( const QString& path, paths )
        m_currScannerPaths.insert( path );

    if ( m_musicScannerThreadController ) //still running if these are not zero
    {
        if ( m_queuedScanType == MusicScanner::None )
            m_queuedScanType = MusicScanner::File;
        tDebug( LOGVERBOSE ) << "Could not run file scan, old scan still running";
        return;
    }

    m_scanTimer->stop();
    m_musicScannerThreadController = new MusicScannerThreadController( this );
    m_currScanMode = MusicScanner::FileScan;
    m_updateGUI = updateGUI;

    QMetaObject::invokeMethod( this, "runScan", Qt::QueuedConnection );
}


void
ScanManager::fileMtimesCheck( const QMap< QString, QMap< unsigned int, unsigned int > >& mtimes )
{
    if ( !mtimes.isEmpty() && m_currScanMode == MusicScanner::DirScan && TomahawkSettings::instance()->scannerPaths().isEmpty() )
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

    QStringList paths = m_currScannerPaths.empty() ? TomahawkSettings::instance()->scannerPaths() : m_currScannerPaths.toList();

    m_musicScannerThreadController->setScanMode( m_currScanMode );
    m_musicScannerThreadController->setPaths( paths );
    m_musicScannerThreadController->start( QThread::IdlePriority );
}


void
ScanManager::scannerFinished()
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( m_musicScannerThreadController )
    {
        m_musicScannerThreadController->quit();
        m_musicScannerThreadController->wait( 60000 );

        delete m_musicScannerThreadController;
        m_musicScannerThreadController = 0;
    }

    SourceList::instance()->getLocal()->scanningFinished( m_updateGUI );
    m_updateGUI = true;
    emit finished();

    if ( m_queuedScanType != MusicScanner::File )
        m_currScannerPaths.clear();
    switch ( m_queuedScanType )
    {
        case MusicScanner::Full:
        case MusicScanner::Normal:
            QMetaObject::invokeMethod( this, "runNormalScan", Qt::QueuedConnection, Q_ARG( bool, m_queuedScanType == MusicScanner::Full ) );
            break;
        case MusicScanner::File:
            QMetaObject::invokeMethod( this, "", Qt::QueuedConnection, Q_ARG( QStringList, QStringList() ) );
            break;
        default:
            break;
    }
    m_queuedScanType = MusicScanner::None;

    m_scanTimer->start();
}
