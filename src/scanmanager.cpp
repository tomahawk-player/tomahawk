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

#include "musicscanner.h"
#include "tomahawksettings.h"
#include "tomahawkutils.h"

ScanManager* ScanManager::s_instance = 0;


ScanManager*
ScanManager::instance()
{
    return s_instance;
}


ScanManager::ScanManager( QObject* parent )
    : QObject( parent )
    , m_scanner( 0 )
    , m_musicScannerThreadController( 0 )
{
    s_instance = this;

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );
    
    if ( TomahawkSettings::instance()->hasScannerPath() )
        m_currScannerPath = TomahawkSettings::instance()->scannerPath();
}


ScanManager::~ScanManager()
{
    qDebug() << Q_FUNC_INFO;
    
    if( m_musicScannerThreadController )
    {
        m_musicScannerThreadController->quit();
    
        while( !m_musicScannerThreadController->isFinished() )
        {
            QCoreApplication::processEvents( QEventLoop::AllEvents, 200 );
            TomahawkUtils::Sleep::msleep( 100 );
        }
        
        if( m_scanner )
        {
            delete m_scanner;
            m_scanner = 0;
        }
        
        delete m_musicScannerThreadController;
        m_musicScannerThreadController = 0;
    }
}


void
ScanManager::onSettingsChanged()
{
    if ( TomahawkSettings::instance()->hasScannerPath() &&
         m_currScannerPath != TomahawkSettings::instance()->scannerPath() )
    {
        m_currScannerPath = TomahawkSettings::instance()->scannerPath();
        runManualScan( m_currScannerPath );
    }
}


void
ScanManager::runManualScan( const QStringList& path )
{
    qDebug() << Q_FUNC_INFO;
    
    if ( !m_musicScannerThreadController && !m_scanner ) //still running if these are not zero
    {
        m_musicScannerThreadController = new QThread( this );
        m_scanner = new MusicScanner( path );
        m_scanner->moveToThread( m_musicScannerThreadController );
        connect( m_scanner, SIGNAL( finished() ), SLOT( scannerFinished() ) );
        m_musicScannerThreadController->start( QThread::IdlePriority );
        QMetaObject::invokeMethod( m_scanner, "startScan" );
    }
    else
        qDebug() << "Could not run manual scan, old scan still running";
}


void
ScanManager::scannerFinished()
{
    qDebug() << Q_FUNC_INFO;
    connect( m_musicScannerThreadController, SIGNAL( finished() ), SLOT( scannerQuit() ) );
    m_musicScannerThreadController->quit();
}


void
ScanManager::scannerQuit()
{
    qDebug() << Q_FUNC_INFO;
    connect( m_scanner, SIGNAL( destroyed( QObject* ) ), SLOT( scannerDestroyed( QObject* ) ) );
    delete m_scanner;
    m_scanner = 0;
}


void
ScanManager::scannerDestroyed( QObject* scanner )
{
    qDebug() << Q_FUNC_INFO;
    m_musicScannerThreadController->deleteLater();
    m_musicScannerThreadController = 0;
    emit finished();
}

