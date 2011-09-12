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

#ifndef MUSICSCANNER_H
#define MUSICSCANNER_H

#include <tomahawksettings.h>

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
#include <QWeakPointer>

// descend dir tree comparing dir mtimes to last known mtime
// emit signal for any dir with new content, so we can scan it.
// finally, emit the list of new mtimes we observed.
class DirLister : public QObject
{
Q_OBJECT

public:

    enum Mode {
        NonRecursive,
        Recursive,
        MTimeOnly
    };

    DirLister( const QStringList& dirs, const QMap<QString, unsigned int>& dirmtimes, TomahawkSettings::ScannerMode mode, bool manualFull, bool recursive )
        : QObject(), m_dirs( dirs ), m_dirmtimes( dirmtimes ), m_mode( mode ), m_manualFull( manualFull ), m_recursive( recursive ), m_opcount( 0 ), m_deleting( false )
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
    void finished( const QMap<QString, unsigned int>& );

private slots:
    void go();
    void scanDir( QDir dir, int depth, DirLister::Mode mode );

private:
    QStringList m_dirs;
    QMap<QString, unsigned int> m_dirmtimes;
    TomahawkSettings::ScannerMode m_mode;
    bool m_manualFull;
    bool m_recursive;

    QMap<QString, unsigned int> m_newdirmtimes;

    uint m_opcount;
    QMutex m_deletingMutex;
    bool m_deleting;
};


class MusicScanner : public QObject
{
Q_OBJECT

public:
    MusicScanner( const QStringList& dirs, TomahawkSettings::ScannerMode mode, bool manualFull, bool recursive = true, quint32 bs = 0 );
    ~MusicScanner();

signals:
    //void fileScanned( QVariantMap );
    void finished();
    void batchReady( const QVariantList&, const QVariantList& );

private:
    QVariant readFile( const QFileInfo& fi );

private slots:
    void listerFinished( const QMap<QString, unsigned int>& newmtimes );
    void scanFile( const QFileInfo& fi );
    void startScan();
    void scan();
    void setDirMtimes( const QMap< QString, unsigned int >& m );
    void setFileMtimes( const QMap< QString, QMap< unsigned int, unsigned int > >& m );
    void commitBatch( const QVariantList& tracks, const QVariantList& deletethese );

private:
    QStringList m_dirs;
    TomahawkSettings::ScannerMode m_mode;
    QMap<QString, QString> m_ext2mime; // eg: mp3 -> audio/mpeg
    unsigned int m_scanned;
    unsigned int m_skipped;

    QList<QString> m_skippedFiles;

    QMap<QString, unsigned int> m_dirmtimes;
    QMap<QString, QMap< unsigned int, unsigned int > > m_filemtimes;
    QMap<QString, unsigned int> m_newdirmtimes;

    QVariantList m_scannedfiles;
    QVariantList m_filesToDelete;
    bool m_manualFull;
    bool m_recursive;
    quint32 m_batchsize;

    QWeakPointer< DirLister > m_dirLister;
    QThread* m_dirListerThreadController;
};

#endif
