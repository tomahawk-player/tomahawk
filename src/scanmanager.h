#ifndef SCANMANAGER_H
#define SCANMANAGER_H

#include <QObject>

#include "dllmacro.h"

class MusicScanner;
class QThread;

class ScanManager : public QObject
{
Q_OBJECT

public:
    static ScanManager* instance();

    explicit ScanManager( QObject* parent = 0 );
    virtual ~ScanManager();
    
    void runManualScan( const QString& path );

private slots:
    void scannerFinished();
    void scannerDestroyed( QObject* scanner );

    void onSettingsChanged();
    
private:
    static ScanManager* s_instance;
    
    MusicScanner* m_scanner;
    QThread* m_musicScannerThreadController;
};

#endif
