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
{
    s_instance = this;

    m_scanTimer = new QTimer( this );
    m_scanTimer->setSingleShot( false );
    m_scanTimer->setInterval( 10000 );

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );
    connect( m_scanTimer, SIGNAL( timeout() ), SLOT( scanTimerTimeout() ) );

    // FIXME: Disable this until we find something nondeprecated and working (e.g. not QFileSystemWatcher)
    //TomahawkSettings::instance()->setWatchForChanges( true );
    
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
        QMetaObject::invokeMethod( m_scanner.data(), "deleteLater", Qt::QueuedConnection );
        while( !m_scanner.isNull() )
        {
            qDebug() << Q_FUNC_INFO << " scanner not delete";
            TomahawkUtils::Sleep::msleep( 50 );
        }

        if ( m_musicScannerThreadController )
            m_musicScannerThreadController->quit();

        if( m_musicScannerThreadController )
        {
            while( !m_musicScannerThreadController->isFinished() )
            {
                qDebug() << Q_FUNC_INFO << " scanner thread controller not finished";
                TomahawkUtils::Sleep::msleep( 50 );
            }

            delete m_musicScannerThreadController;
            m_musicScannerThreadController = 0;
        }
    }
}


void
ScanManager::onSettingsChanged()
{
    if ( !TomahawkSettings::instance()->watchForChanges() && m_scanTimer->isActive() )
        m_scanTimer->stop();
    
    if ( TomahawkSettings::instance()->hasScannerPaths() &&
        m_currScannerPaths != TomahawkSettings::instance()->scannerPaths() )
    {
        m_currScannerPaths = TomahawkSettings::instance()->scannerPaths();
        runManualScan( m_currScannerPaths );
    }

    if ( TomahawkSettings::instance()->watchForChanges() && !m_scanTimer->isActive() )
        m_scanTimer->start();
}


void
ScanManager::runStartupScan()
{
    qDebug() << Q_FUNC_INFO;
    if( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        QTimer::singleShot( 1000, this, SLOT( runStartupScan() ) );
    else
        runManualScan( m_currScannerPaths );
}


void
ScanManager::scanTimerTimeout()
{
    qDebug() << Q_FUNC_INFO;
    if( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        return;
    else
        runManualScan( m_currScannerPaths );
}


void
ScanManager::runManualScan( const QStringList& paths )
{
    qDebug() << Q_FUNC_INFO;

    if( !Database::instance() || ( Database::instance() && !Database::instance()->isReady() ) )
        return;

    if ( !m_musicScannerThreadController && m_scanner.isNull() ) //still running if these are not zero
    {
        m_musicScannerThreadController = new QThread( this );
        m_scanner = QWeakPointer< MusicScanner>( new MusicScanner( paths ) );
        m_scanner.data()->moveToThread( m_musicScannerThreadController );
        connect( m_scanner.data(), SIGNAL( finished() ), SLOT( scannerFinished() ) );
        m_musicScannerThreadController->start( QThread::IdlePriority );
        QMetaObject::invokeMethod( m_scanner.data(), "startScan" );
    }
    else
    {
        qDebug() << "Could not run manual scan, old scan still running";
        return;
    }
}


void
ScanManager::scannerFinished()
{
    if ( !m_scanner.isNull() )
    {
        QMetaObject::invokeMethod( m_scanner.data(), "deleteLater", Qt::QueuedConnection );
        while( !m_scanner.isNull() )
        {
            qDebug() << Q_FUNC_INFO << " scanner not deleted, processing events";
            QCoreApplication::processEvents( QEventLoop::AllEvents, 200 );
            TomahawkUtils::Sleep::msleep( 100 );
        }

        if ( m_musicScannerThreadController )
            m_musicScannerThreadController->quit();

        if( m_musicScannerThreadController )
        {
            while( !m_musicScannerThreadController->isFinished() )
            {
                qDebug() << Q_FUNC_INFO << " scanner thread controller not finished, processing events";
                QCoreApplication::processEvents( QEventLoop::AllEvents, 200 );
                TomahawkUtils::Sleep::msleep( 100 );
            }

            delete m_musicScannerThreadController;
            m_musicScannerThreadController = 0;
        }
    }
    emit finished();
}
