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

#include <QThread>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QTimer>

#include "musicscanner.h"
#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "libtomahawk/sourcelist.h"

#include "database/database.h"
#include "database/databasecommand_dirmtimes.h"
#include "database/databasecommand_deletefiles.h"

#include "utils/logger.h"

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
{
    s_instance = this;

    m_scanTimer = new QTimer( this );
    m_scanTimer->setSingleShot( false );
    m_scanTimer->setInterval( TomahawkSettings::instance()->scannerTime() * 1000 );

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );
    connect( m_scanTimer, SIGNAL( timeout() ), SLOT( scanTimerTimeout() ) );

    if ( TomahawkSettings::instance()->hasScannerPaths() )
    {
        m_currScannerPaths = TomahawkSettings::instance()->scannerPaths();
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
    qDebug() << Q_FUNC_INFO << " scanner thread controller finished, exiting ScanManager";
}


void
ScanManager::onSettingsChanged()
{
    if ( !TomahawkSettings::instance()->watchForChanges() && m_scanTimer->isActive() )
        m_scanTimer->stop();

    m_scanTimer->setInterval( TomahawkSettings::instance()->scannerTime() * 1000 );

    if ( TomahawkSettings::instance()->hasScannerPaths() &&
        m_currScannerPaths != TomahawkSettings::instance()->scannerPaths() )
    {
        m_currScannerPaths = TomahawkSettings::instance()->scannerPaths();
        runScan();
    }

    if ( TomahawkSettings::instance()->watchForChanges() && !m_scanTimer->isActive() )
        m_scanTimer->start();
}


void
ScanManager::runStartupScan()
{
    qDebug() << Q_FUNC_INFO;
    if ( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        QTimer::singleShot( 1000, this, SLOT( runStartupScan() ) );
    else
        runScan();
}


void
ScanManager::scanTimerTimeout()
{
    qDebug() << Q_FUNC_INFO;
    if ( !TomahawkSettings::instance()->watchForChanges() ||
         !Database::instance() ||
         ( Database::instance() && !Database::instance()->isReady() ) )
        return;
    else
        runScan();
}


void
ScanManager::runScan( bool manualFull )
{
    qDebug() << Q_FUNC_INFO;
    if ( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        return;

    if ( !manualFull )
    {
        runDirScan( TomahawkSettings::instance()->scannerPaths(), false );
        return;
    }

    DatabaseCommand_DirMtimes *cmd = new DatabaseCommand_DirMtimes( QMap<QString, unsigned int>() );
    connect( cmd, SIGNAL( done( QMap< QString, unsigned int > ) ),
                          SLOT( mtimesDeleted( QMap< QString, unsigned int > ) ) );

    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );   
}


void
ScanManager::mtimesDeleted( QMap< QString, unsigned int > returnedMap )
{
    Q_UNUSED( returnedMap );
    qDebug() << Q_FUNC_INFO;
    DatabaseCommand_DeleteFiles *cmd = new DatabaseCommand_DeleteFiles( SourceList::instance()->getLocal() );
    connect( cmd, SIGNAL( done( const QStringList&, const Tomahawk::collection_ptr& ) ),
                          SLOT( filesDeleted( const QStringList&, const Tomahawk::collection_ptr& ) ) );
    
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
}


void
ScanManager::filesDeleted( const QStringList& files, const Tomahawk::collection_ptr& collection )
{
    Q_UNUSED( files );
    Q_UNUSED( collection );
    runDirScan( TomahawkSettings::instance()->scannerPaths(), true );
}


void
ScanManager::runDirScan( const QStringList& paths, bool manualFull )
{
    qDebug() << Q_FUNC_INFO;

    if ( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        return;

    if ( paths.isEmpty() )
    {
        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( new DatabaseCommand_DeleteFiles( SourceList::instance()->getLocal() ) ) );
        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( new DatabaseCommand_DirMtimes( QMap<QString, unsigned int>() ) ) );
        return;
    }

    if ( !m_musicScannerThreadController && m_scanner.isNull() ) //still running if these are not zero
    {
        m_scanTimer->stop();
        m_musicScannerThreadController = new QThread( this );
        m_scanner = QWeakPointer< MusicScanner>( new MusicScanner( paths, manualFull ? TomahawkSettings::Dirs : TomahawkSettings::instance()->scannerMode() ) );
        m_scanner.data()->moveToThread( m_musicScannerThreadController );
        connect( m_scanner.data(), SIGNAL( finished() ), SLOT( scannerFinished() ) );
        m_musicScannerThreadController->start( QThread::IdlePriority );
        QMetaObject::invokeMethod( m_scanner.data(), "startScan" );
    }
    else
    {
        qDebug() << "Could not run dir scan, old scan still running";
        return;
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
    m_scanTimer->start();
    SourceList::instance()->getLocal()->scanningFinished( 0 );
    emit finished();
}
