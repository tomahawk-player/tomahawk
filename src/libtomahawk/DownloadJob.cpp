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

#include "DownloadJob.h"

#include "Result.h"
#include "Track.h"
#include "Result.h"
#include "resolvers/ScriptResolver.h"
#include "resolvers/ScriptCollection.h"
#include "resolvers/ScriptObject.h"
#include "resolvers/ScriptJob.h"
#include "TomahawkSettings.h"
#include "utils/NetworkAccessManager.h"
#include "utils/Logger.h"


DownloadJob::DownloadJob( const Tomahawk::result_ptr& result, DownloadFormat format, bool tryResuming, DownloadJob::TrackState state )
    : m_state( state )
    , m_retries( 0 )
    , m_tryResuming( tryResuming )
    , m_reply( 0 )
    , m_file( 0 )
    , m_rcvdStamp( 0 )
    , m_rcvdEmit( 0 )
    , m_rcvdSize( 0 )
    , m_fileSize( 0 )
    , m_format( format )
    , m_track( result->track() )
    , m_result( result )
{
    m_finished = ( state == Finished );
}


DownloadJob::~DownloadJob()
{
}


void
DownloadJob::onUrlRetrieved( const QVariantMap& data )
{
    tDebug() << Q_FUNC_INFO << data;
    QUrl localFile = prepareFilename();
    if ( m_file )
    {
        tLog() << "Recovering from failed download for track:" << toString() << "-" << m_retries << "retries so far.";
        m_finished = false;
        delete m_file;
        m_file = 0;
    }

    tLog() << "Saving download" << m_format.url << "to file:" << localFile << localFile.toLocalFile();

    m_file = new QFile( localFile.toString() );
    m_localFile = localFile.toString();

    if ( m_tryResuming && checkForResumedFile() )
        return;

    m_reply = Tomahawk::Utils::nam()->get( QNetworkRequest( data[ "url" ].toString() ) );

    connect( m_reply, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( onDownloadError( QNetworkReply::NetworkError ) ) );
    connect( m_reply, SIGNAL( downloadProgress( qint64, qint64 ) ), SLOT( onDownloadProgress( qint64, qint64 ) ) );
    connect( m_reply, SIGNAL( finished() ), SLOT( onDownloadNetworkFinished() ) );

    setState( Running );
}


int
DownloadJob::progressPercentage() const
{
    if ( m_fileSize == 0 )
        return 0;

    return ( (double)m_rcvdSize / (double)m_fileSize ) * 100.0;
}


void
DownloadJob::setState( TrackState state )
{
    TrackState oldState = m_state;
    m_state = state;

    emit stateChanged( state, oldState );

    if ( m_state == Finished )
    {
        m_rcvdSize = m_fileSize;
        emit finished();
    }
}


QString
DownloadJob::localFile() const
{
    return m_localFile;
}


QString
DownloadJob::localPath( const Tomahawk::album_ptr& album )
{
    QDir dir = TomahawkSettings::instance()->downloadsPath();

    if ( !dir.exists() )
    {
        dir.mkpath( "." );
    }

    QString path = QString( "%1/%2" ).arg( safeEncode( album->artist()->name(), true ) ).arg( safeEncode( album->name(), true ) );
    dir.mkpath( path );

    return QString( dir.path() + "/" + path ).replace( "//", "/" );
}


QUrl
DownloadJob::prepareFilename()
{
    QString filename = QString( "%1. %2.%3" ).arg( m_track->albumpos() ).arg( safeEncode( m_track->track() ) ).arg( m_format.extension );
    QString path = localPath( m_track->albumPtr() );
    QString localFile = QString( path + "/" + filename );

    if ( !m_tryResuming )
    {
        QFileInfo fileInfo( localFile );
        unsigned int dupe = 1;
        while ( dupe < 100 )
        {
            QFileInfo fi( localFile );
            if ( fi.exists() )
            {
                QString dupeToken = QString( " (%1)" ).arg( dupe++ );
                localFile = path + "/" + fileInfo.completeBaseName() + dupeToken + "." + fileInfo.suffix();
            }
            else
                break;
        }
    }
    else
    {
        QFileInfo fileInfo( localFile );
        unsigned int dupe = 1;
        QString lastFound = localFile;
        while ( dupe < 100 )
        {
            QFileInfo fi( localFile );
            if ( fi.exists() )
            {
                lastFound = localFile;
                QString dupeToken = QString( " (%1)" ).arg( dupe++ );
                localFile = path + "/" + fileInfo.completeBaseName() + dupeToken + "." + fileInfo.suffix();
            }
            else
                break;
        }

        localFile = lastFound;
    }

    tLog() << "Storing file as" << localFile << m_tryResuming;
    return QUrl( localFile );
}


void
DownloadJob::retry()
{
    tLog() << Q_FUNC_INFO;

    m_retries = 0;
    m_reply = 0;
    m_file = 0;
    m_rcvdSize = 0;
    m_fileSize = 0;
    m_finished = false;
    m_tryResuming = true;

    setState( Waiting );
    emit updated();
}


bool
DownloadJob::download()
{
    if ( m_state == Running )
        return true;

    setState( Running );

    if (m_result->resolvedBy() != nullptr) {
        Tomahawk::ScriptJob *job = m_result->resolvedBy()->getDownloadUrl( m_result, m_format );
        connect( job, SIGNAL( done(QVariantMap) ), SLOT( onUrlRetrieved(QVariantMap) ) );
        job->start();
    } else {
        onUrlRetrieved({{"url", m_format.url}});
    }

    return true;
}


void
DownloadJob::pause()
{
    if ( !m_reply )
        return;

    setState( Paused );
//    m_reply->setReadBufferSize( 0 );
}


void
DownloadJob::resume()
{
    tLog() << Q_FUNC_INFO << m_finished << m_rcvdSize << m_fileSize;
    if ( !m_reply )
    {
        tLog() << "Initiating paused download from previous session:" << toString();
        download();
        return;
    }

    setState( Running );
//    m_reply->setReadBufferSize( 65536 );

    onDownloadProgress( m_fileSize, m_fileSize );
}


void
DownloadJob::abort()
{
    tLog() << Q_FUNC_INFO << toString();
    setState( Aborted );

    if ( m_reply )
    {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = 0;
    }
}


void
DownloadJob::onDownloadError( QNetworkReply::NetworkError code )
{
    if ( code == QNetworkReply::NoError )
        return;
    if ( m_state == Aborted )
        return;

    tLog() << "Download error for track:" << toString() << "-" << code;

    if ( ++m_retries > 3 )
    {
        setState( Failed );
    }
    else
    {
        m_tryResuming = true;
        download();
    }
}


void
DownloadJob::onDownloadProgress( qint64 rcvd, qint64 total )
{
    if ( m_reply == 0 )
        return;

    if ( rcvd >= m_fileSize && m_fileSize > 0 )
    {
        m_finished = true;
    }

    if ( state() == Paused )
        return;

    m_rcvdSize = rcvd;
    m_fileSize = total;

    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if ( ( now - 50 > m_rcvdStamp ) || ( rcvd == total ) )
    {
        m_rcvdStamp = now;
        if ( ( m_rcvdSize - 16384 > m_rcvdEmit ) || ( rcvd == total ) )
        {
            m_rcvdEmit = m_rcvdSize;
            emit progress( progressPercentage() );
        }
    }

    if ( !m_file )
        return;

    if ( !m_file->isOpen() )
    {
        if ( m_tryResuming && checkForResumedFile() )
            return;

        if ( !m_file->open( QIODevice::WriteOnly ) )
        {
            tLog() << Q_FUNC_INFO << "Failed opening file:" << m_file->fileName();
            setState( Failed );
            return;
        }
    }

    QByteArray data = m_reply->readAll();
    if ( data.isEmpty() )
        return;

    if ( m_file->write( data ) < 0 )
    {
        tLog() << Q_FUNC_INFO << "Failed writing to file:" << data.length();
        onDownloadError( QNetworkReply::UnknownContentError );
        return;
    }

    if ( m_rcvdSize >= m_fileSize && m_fileSize > 0 )
    {
        onDownloadFinished();
    }
    else if ( m_reply->isFinished() && m_reply->bytesAvailable() == 0 )
    {
        if ( ( m_fileSize > 0 && m_rcvdSize < m_fileSize ) || m_rcvdSize == 0 )
        {
            onDownloadError( QNetworkReply::UnknownContentError );
            return;
        }
    }
}


void
DownloadJob::onDownloadNetworkFinished()
{
    tLog() << Q_FUNC_INFO << m_rcvdSize << m_fileSize;
    if ( m_reply && m_reply->bytesAvailable() > 0 )
    {
        tLog() << "Expecting more data!";
        return;
    }

    if ( ( m_fileSize > 0 && m_rcvdSize < m_fileSize ) || m_rcvdSize == 0 )
    {
        if ( m_reply )
            onDownloadError( QNetworkReply::UnknownContentError );
        return;
    }
}


void
DownloadJob::onDownloadFinished()
{
    tLog() << Q_FUNC_INFO << m_rcvdSize << m_fileSize;
    if ( state() == Paused )
    {
        m_finished = true;
        return;
    }

    if ( ( m_fileSize > 0 && m_rcvdSize < m_fileSize ) || m_rcvdSize == 0 )
    {
        onDownloadError( QNetworkReply::UnknownContentError );
        return;
    }

    disconnect( m_reply, SIGNAL( error( QNetworkReply::NetworkError ) ), this, SLOT( onDownloadError( QNetworkReply::NetworkError ) ) );
    disconnect( m_reply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( onDownloadProgress( qint64, qint64 ) ) );
    disconnect( m_reply, SIGNAL( finished() ), this, SLOT( onDownloadNetworkFinished() ) );
    m_reply->abort();

    if ( m_file && m_file->isOpen() )
    {
        m_file->flush();
        m_file->close();
    }

    delete m_file;
    m_file = 0;

    m_finishedTimestamp = QDateTime::currentDateTimeUtc();
    setState( Finished );
    tLog() << Q_FUNC_INFO << "Finished downloading:" << toString();
    tLog() << Q_FUNC_INFO << m_finished;
}


bool
DownloadJob::checkForResumedFile()
{
    tLog() << Q_FUNC_INFO;
    if ( !m_tryResuming )
        return false;

    QUrl localFile = prepareFilename();
    QFileInfo fi( localFile.toString() );

    tLog() << Q_FUNC_INFO << m_fileSize << fi.size();

    if ( fi.size() > 0 && fi.exists() && fi.size() == m_fileSize )
    {
        tLog() << Q_FUNC_INFO << "Detected previously finished download.";
        m_rcvdSize = fi.size();
        m_finished = true;
        onDownloadFinished();
        return true;
    }

    return false;
}


QString
DownloadJob::safeEncode( const QString& filename, bool removeTrailingDots )
{
    //FIXME: make it a regexp
    QString res = QString( filename ).toLatin1().replace( "/", "_" ).replace( "\\", "_" )
                                     .replace( "*", "_" ).replace( "?", "_" ).replace( "%", "_" )
                                     .replace( "'", "_" ).replace( "\"", "_" )
                                     .replace( ":", "_" ).replace( "#", "_" )
                                     .replace( "<", "_" ).replace( ">", "_" );

    if ( removeTrailingDots )
    {
        while ( res.endsWith( "." ) )
            res = res.left( res.count() - 1 );
    }

    return res.left( 127 );
}


QString
DownloadJob::toString() const
{
    return m_track->toString();
}


DownloadFormat
DownloadJob::format() const
{
    return m_format;
}
