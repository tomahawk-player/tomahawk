#include "scanmanager.h"

#include <QDebug>
#include <QThread>

#include "musicscanner.h"
#include "tomahawksettings.h"

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
}


ScanManager::~ScanManager()
{
    qDebug() << Q_FUNC_INFO;
    s_instance = 0;
    if( m_musicScannerThreadController )
    {
        m_musicScannerThreadController->quit();
        m_musicScannerThreadController->deleteLater();
    }
    m_musicScannerThreadController = 0;
    m_scanner = 0;    
}


void
ScanManager::onSettingsChanged()
{
    if ( TomahawkSettings::instance()->hasScannerPath() )
        runManualScan( TomahawkSettings::instance()->scannerPath() );
}


void
ScanManager::runManualScan( const QString& path )
{
    qDebug() << Q_FUNC_INFO;
    if ( !m_musicScannerThreadController && !m_scanner ) //still running if these are not zero
    {
        m_musicScannerThreadController = new QThread( this );
        m_scanner = new MusicScanner( path );
        m_scanner->moveToThread( m_musicScannerThreadController );
        connect( m_scanner, SIGNAL( finished() ), SLOT( scannerFinished() ) );
        connect( m_scanner, SIGNAL( destroyed( QObject* ) ), SLOT( scannerDestroyed( QObject* ) ) );
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
    if( m_musicScannerThreadController->isRunning() )
    {
        qDebug() << "Scan thread still running, not deleting yet";
        m_musicScannerThreadController->quit();
        QMetaObject::invokeMethod( this, "scannerFinished" );
    }
    else
    {
        delete m_scanner;
        m_scanner = 0;
    }
}

void
ScanManager::scannerDestroyed( QObject* scanner )
{
    qDebug() << Q_FUNC_INFO;
    m_musicScannerThreadController->deleteLater();
    m_musicScannerThreadController = 0;
}

