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

#ifndef SCANMANAGER_H
#define SCANMANAGER_H

#include "Typedefs.h"
#include "DllMacro.h"

#include "MusicScanner.h"

#include <QHash>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QPointer>
#include <QSet>
#include <QThread>

class QFileSystemWatcher;
class QTimer;

class MusicScannerThreadController : public QThread
{
    Q_OBJECT

public:
    MusicScannerThreadController( QObject* parent );
    virtual ~MusicScannerThreadController();

    void setScanMode( MusicScanner::ScanMode mode ) { m_mode = mode; }
    void setPaths( const QStringList& paths ) { m_paths = paths; }
    void run();

private:
    QPointer< MusicScanner > m_musicScanner;
    MusicScanner::ScanMode m_mode;
    QStringList m_paths;
    quint32 m_bs; 
};


class DLLEXPORT ScanManager : public QObject
{
Q_OBJECT

public:
    static ScanManager* instance();

    explicit ScanManager( QObject* parent = 0 );
    virtual ~ScanManager();

signals:
    void finished();

public slots:
    void runFileScan( const QStringList& paths = QStringList(), bool updateGUI = true );
    void runFullRescan();
    void runNormalScan( bool manualFull = false );

private slots:
    void runStartupScan();
    void runScan();

    void scannerFinished();
    void scanTimerTimeout();

    void onSettingsChanged();

    void fileMtimesCheck( const QMap< QString, QMap< unsigned int, unsigned int > >& mtimes );
    void filesDeleted();

private:
    static ScanManager* s_instance;

    MusicScanner::ScanMode m_currScanMode;
    MusicScannerThreadController* m_musicScannerThreadController;
    QSet< QString > m_currScannerPaths;
    QStringList m_cachedScannerDirs;

    QTimer* m_scanTimer;
    MusicScanner::ScanType m_queuedScanType;

    bool m_updateGUI;
};

#endif
