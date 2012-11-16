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

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QWeakPointer>
#include <QtCore/QSet>

class MusicScanner;
class QThread;
class QFileSystemWatcher;
class QTimer;

class DLLEXPORT ScanManager : public QObject
{
Q_OBJECT

public:
    enum ScanMode { DirScan, FileScan };
    enum ScanType { None, Full, Normal, File };

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

    ScanMode m_currScanMode;
    QWeakPointer< MusicScanner > m_scanner;
    QThread* m_musicScannerThreadController;
    QSet< QString > m_currScannerPaths;
    QStringList m_cachedScannerDirs;

    QTimer* m_scanTimer;
    ScanType m_queuedScanType;

    bool m_updateGUI;
};

#endif
