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

#ifndef MUSICSCANNER_H
#define MUSICSCANNER_H

#include "database/Database.h"
#include "database/DatabaseCommand.h"
#include "TomahawkSettings.h"

/* taglib */
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVariantMap>

struct libvlc_instance_t;

// descend dir tree comparing dir mtimes to last known mtime
// emit signal for any dir with new content, so we can scan it.
// finally, emit the list of new mtimes we observed.
class DirLister : public QObject
{
Q_OBJECT

public:

    DirLister( const QStringList& dirs )
        : QObject(), m_dirs( dirs ), m_opcount( 0 ), m_deleting( false )
    {
        qDebug() << Q_FUNC_INFO;
    }

    ~DirLister()
    {
        qDebug() << Q_FUNC_INFO;
    }

    bool isDeleting() { QMutexLocker locker( &m_deletingMutex ); return m_deleting; };
    void setIsDeleting() { QMutexLocker locker( &m_deletingMutex ); m_deleting = true; };

signals:
    void fileToScan( QFileInfo );
    void finished();

private slots:
    void go();
    void scanDir( QDir dir, int depth );

private:
    QStringList m_dirs;
    QSet< QString > m_processedDirs;

    uint m_opcount;
    QMutex m_deletingMutex;
    bool m_deleting;
};

class DirListerThreadController : public QThread
{
    Q_OBJECT

public:
    DirListerThreadController( QObject* parent );
    virtual ~DirListerThreadController();

    void setPaths( const QStringList& paths ) { m_paths = paths; }
    void run();

private:
    QPointer< DirLister > m_dirLister;
    QStringList m_paths;
};

class DLLEXPORT MusicScanner : public QObject
{
Q_OBJECT

public:
    enum ScanMode { DirScan, FileScan };
    enum ScanType { None, Full, Normal, File };

    static QVariant readTags( const QFileInfo& fi, libvlc_instance_t* vlcInstance );

    MusicScanner( MusicScanner::ScanMode scanMode, const QStringList& paths, libvlc_instance_t* pVlcInstance, quint32 bs = 0 );
    ~MusicScanner();

    /**
     * Specify if we want a dry run, i.e. not change any of the internal data stores.
     *
     * This is useful for testing if the scanner works (while Tomahawk is running).
     */
    void setDryRun( bool _dryRun );
    bool dryRun();

    /**
     * Adjust the verbosity of logging.
     *
     * For example in verbose mode we will log each file we have scanned.
     */
    void setVerbose( bool _verbose );
    bool verbose();

signals:
    //void fileScanned( QVariantMap );
    void finished();
    void batchReady( const QVariantList&, const QVariantList& );
    void progress( unsigned int files );

private:
    QVariant readFile( const QFileInfo& fi );
    void executeCommand( Tomahawk::dbcmd_ptr cmd );

private slots:
    void postOps();
    void scanFile( const QFileInfo& fi );
    void setFileMtimes( const QMap< QString, QMap< unsigned int, unsigned int > >& m );
    void startScan();
    void scan();
    void cleanup();
    void commitBatch( const QVariantList& tracks, const QVariantList& deletethese );
    void commandFinished();

private:
    void scanFilePaths();

    MusicScanner::ScanMode m_scanMode;
    QStringList m_paths;
    unsigned int m_scanned;
    unsigned int m_skipped;
    bool m_dryRun;
    bool m_verbose;

    QList<QString> m_skippedFiles;
    QMap<QString, QMap< unsigned int, unsigned int > > m_filemtimes;

    unsigned int m_cmdQueue;

    QSet< QString > m_processedFiles;
    QVariantList m_scannedfiles;
    QVariantList m_filesToDelete;
    quint32 m_batchsize;
    libvlc_instance_t* m_vlcInstance;

    DirListerThreadController* m_dirListerThreadController;
};

#endif
