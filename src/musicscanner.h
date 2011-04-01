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

/* taglib */
#include <fileref.h>
#include <tag.h>

#include <QVariantMap>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QDebug>
#include <QDateTime>
#include <QTimer>

// descend dir tree comparing dir mtimes to last known mtime
// emit signal for any dir with new content, so we can scan it.
// finally, emit the list of new mtimes we observed.
class DirLister : public QObject
{
Q_OBJECT

public:
    DirLister( QDir d, QMap<QString, unsigned int>& mtimes )
        : QObject(), m_dir( d ), m_dirmtimes( mtimes )
    {
        qDebug() << Q_FUNC_INFO;
    }

    ~DirLister()
    {
        qDebug() << Q_FUNC_INFO;
    }

signals:
    void fileToScan( QFileInfo );
    void finished( const QMap<QString, unsigned int>& );

private slots:
    void go();
    void scanDir( QDir dir, int depth );

private:
    QDir m_dir;
    QMap<QString, unsigned int> m_dirmtimes;
    QMap<QString, unsigned int> m_newdirmtimes;
};

class MusicScanner : public QObject
{
Q_OBJECT

public:
    MusicScanner( const QStringList& dirs, quint32 bs = 0 );
    ~MusicScanner();

signals:
    //void fileScanned( QVariantMap );
    void finished();
    void batchReady( const QVariantList& );
    void addWatchedDirs( const QStringList & );
    void removeWatchedDir( const QString & );

private:
    QVariant readFile( const QFileInfo& fi );

private slots:
    void listerFinished( const QMap<QString, unsigned int>& newmtimes );
    void deleteLister();
    void listerQuit();
    void listerDestroyed( QObject* dirLister );
    void scanFile( const QFileInfo& fi );
    void startScan();
    void scan();
    void setMtimes( const QMap<QString, unsigned int>& m );
    void commitBatch( const QVariantList& );

private:
    QStringList m_dirs;
    QMap<QString, QString> m_ext2mime; // eg: mp3 -> audio/mpeg
    unsigned int m_scanned;
    unsigned int m_skipped;

    QList<QString> m_skippedFiles;

    QMap<QString, unsigned int> m_dirmtimes;
    QMap<QString, unsigned int> m_newdirmtimes;

    QList<QVariant> m_scannedfiles;
    quint32 m_batchsize;

    DirLister* m_dirLister;
    QThread* m_dirListerThreadController;
};

#endif
