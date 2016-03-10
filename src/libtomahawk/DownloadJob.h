/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef DOWNLOADJOB_H
#define DOWNLOADJOB_H

#include <QDir>
#include <QObject>
#include <QFile>
#include <QUrl>
#include <QPixmap>
#include <QDomElement>
#include <QNetworkReply>
#include <QDateTime>

#include "Track.h"

struct DownloadFormat
{
    QUrl url;
    QString extension;
    QString mimetype;
};

class DownloadJob : public QObject
{
Q_OBJECT

public:
    enum TrackState
    { Waiting = 0, Running, Paused, Failed, Finished, Aborted, Any };

    DownloadJob( const Tomahawk::result_ptr& result, DownloadFormat format, bool tryResuming = false, DownloadJob::TrackState state = Waiting );
    ~DownloadJob();

    QString toString() const;

    TrackState state() const { return m_state; }
    unsigned int retries() const { return m_retries; }

    Tomahawk::track_ptr track() const { return m_track; }
    int progressPercentage() const;
    long receivedSize() const { return m_rcvdSize; }
    long fileSize() const { return m_fileSize; }

    static QString localPath( const Tomahawk::album_ptr& album );
    QString localFile() const;
    DownloadFormat format() const;

    QDateTime finishedTimestamp() const { return m_finishedTimestamp; }
    void setFinishedTimestamp( const QDateTime& timestamp ) { m_finishedTimestamp = timestamp; }

    void setState( TrackState state );

public slots:
    bool download();
    void pause();
    void resume();
    void retry();
    void abort();

signals:
    void updated();
    void progress( int percentage );
    void stateChanged( DownloadJob::TrackState newState, DownloadJob::TrackState oldState );
    void finished();

private slots:
    void onDownloadNetworkFinished();
    void onDownloadError( QNetworkReply::NetworkError code );
    void onDownloadProgress( qint64, qint64 );
    void onDownloadFinished();

    void onUrlRetrieved( const QVariantMap& data );

private:
    void storeState();
    static QString safeEncode( const QString& filename, bool removeTrailingDots = false );
    bool checkForResumedFile();
    QUrl prepareFilename();

    TrackState m_state;
    unsigned int m_retries;
    bool m_tryResuming;

    QNetworkReply* m_reply;
    QFile* m_file;

    qint64 m_rcvdStamp;
    long m_rcvdEmit;
    long m_rcvdSize;
    long m_fileSize;
    bool m_finished;

    QString m_localFile;

    QDateTime m_finishedTimestamp;

    DownloadFormat m_format;
    Tomahawk::track_ptr m_track;
    Tomahawk::result_ptr m_result;
};

typedef QSharedPointer<DownloadJob> downloadjob_ptr;

#endif // DOWNLOADJOB_H
