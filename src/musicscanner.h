#ifndef MUSICSCANNER_H
#define MUSICSCANNER_H

#include <taglib/fileref.h>
#include <taglib/tag.h>

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
    void go()
    {
        scanDir( m_dir, 0 );
        emit finished( m_newdirmtimes );
    }

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
    MusicScanner( const QString& dir, quint32 bs = 0 );
    ~MusicScanner();

signals:
    //void fileScanned( QVariantMap );
    void finished();
    void batchReady( const QVariantList& );

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
    QString m_dir;
    QMap<QString,QString> m_ext2mime; // eg: mp3 -> audio/mpeg
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
