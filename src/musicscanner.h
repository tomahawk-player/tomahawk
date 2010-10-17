#ifndef MUSICSCANNER_H
#define MUSICSCANNER_H

#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <QVariantMap>
#include <QThread>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QDebug>
#include <QDateTime>

class MusicScanner : public QThread
{
Q_OBJECT

public:
    MusicScanner( const QString& dir, quint32 bs = 0 );

protected:
    void run();

signals:
    //void fileScanned( QVariantMap );
    void finished( int, int );
    void batchReady( const QVariantList& );

private:
    QVariant readFile( const QFileInfo& fi );

private slots:
    void listerFinished( const QMap<QString, unsigned int>& newmtimes );
    void scanFile( const QFileInfo& fi );
    void startScan();
    void scan();
    void setMtimes( const QMap<QString, unsigned int>& m );
    void commitBatch( const QVariantList& );

private:
    QString m_dir;
    QMap<QString,QString> m_ext2mime; // eg: mp3 -> audio/mpeg
    unsigned int m_scanned;
    unsigned int m_skipped;

    QList<QString> m_skippedFiles;

    QMap<QString, unsigned int> m_dirmtimes;
    QMap<QString, unsigned int> m_newdirmtimes;

    QList<QVariant> m_scannedfiles;
    quint32 m_batchsize;
};


#include <QTimer>

// descend dir tree comparing dir mtimes to last known mtime
// emit signal for any dir with new content, so we can scan it.
// finally, emit the list of new mtimes we observed.
class DirLister : public QThread
{
    Q_OBJECT
public:
    DirLister( QDir d, QMap<QString, unsigned int>& mtimes )
        : QThread(), m_dir( d ), m_dirmtimes( mtimes )
    {
        qDebug() << Q_FUNC_INFO;
        moveToThread(this);
    }

    ~DirLister()
    {
        qDebug() << Q_FUNC_INFO;
    }

protected:
    void run()
    {
        QTimer::singleShot(0,this,SLOT(go()));
        exec();
    }

signals:
    void fileToScan( QFileInfo );
    void finished( const QMap<QString, unsigned int>& );

private slots:
    void go()
    {
        scanDir( m_dir, 0 );
        emit finished( m_newdirmtimes );
    }

    void scanDir( QDir dir, int depth )
    {
        QFileInfoList dirs;
        const uint mtime = QFileInfo( dir.absolutePath() ).lastModified().toUTC().toTime_t();
        m_newdirmtimes.insert( dir.absolutePath(), mtime );

        if ( m_dirmtimes.contains( dir.absolutePath() ) &&
             mtime == m_dirmtimes.value( dir.absolutePath() )
           )
        {
            // dont scan this dir, unchanged since last time.
        }
        else
        {
            dir.setFilter( QDir::Files | QDir::Readable | QDir::NoDotAndDotDot );
            dir.setSorting( QDir::Name );
            dirs = dir.entryInfoList();
            foreach( QFileInfo di, dirs )
            {
                emit fileToScan( di );
            }
        }
        dir.setFilter( QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot );
        dirs = dir.entryInfoList();

        foreach( QFileInfo di, dirs )
        {
            scanDir( di.absoluteFilePath(), depth + 1 );
        }
    }

private:
    QDir m_dir;
    QMap<QString, unsigned int> m_dirmtimes;
    QMap<QString, unsigned int> m_newdirmtimes;
};

#endif
