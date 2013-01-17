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

#include "TomahawkSettings.h"
#include "database/DatabaseCommand.h"

/* taglib */
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <QVariantMap>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <database/Database.h>

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

class MusicScanner : public QObject
{
Q_OBJECT

public:
    enum ScanMode { DirScan, FileScan };
    enum ScanType { None, Full, Normal, File };

    MusicScanner( MusicScanner::ScanMode scanMode, const QStringList& paths, quint32 bs = 0 );
    ~MusicScanner();

signals:
    //void fileScanned( QVariantMap );
    void finished();
    void batchReady( const QVariantList&, const QVariantList& );

private:
    QVariant readFile( const QFileInfo& fi );
    void executeCommand( QSharedPointer< DatabaseCommand > cmd );

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
    QMap<QString, QString> m_ext2mime; // eg: mp3 -> audio/mpeg
    unsigned int m_scanned;
    unsigned int m_skipped;

    QList<QString> m_skippedFiles;
    QMap<QString, QMap< unsigned int, unsigned int > > m_filemtimes;

    unsigned int m_cmdQueue;

    QVariantList m_scannedfiles;
    QVariantList m_filesToDelete;
    quint32 m_batchsize;

    DirListerThreadController* m_dirListerThreadController;
};

#endif
