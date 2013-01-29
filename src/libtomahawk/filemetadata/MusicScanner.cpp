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

#include "MusicScanner.h"

#include <QtCore/QCoreApplication>

#include "utils/TomahawkUtils.h"
#include "TomahawkSettings.h"
#include "SourceList.h"
#include "database/Database.h"
#include "database/DatabaseCommand_DirMtimes.h"
#include "database/DatabaseCommand_FileMTimes.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "database/DatabaseCommand_AddFiles.h"
#include "database/DatabaseCommand_DeleteFiles.h"
#include "taghandlers/tag.h"

#include "utils/Logger.h"

void
DirLister::go()
{
    foreach ( const QString& dir, m_dirs )
    {
        m_opcount++;
        QMetaObject::invokeMethod( this, "scanDir", Qt::QueuedConnection, Q_ARG( QDir, QDir( dir, 0 ) ), Q_ARG( int, 0 ) );
    }
}


void
DirLister::scanDir( QDir dir, int depth )
{
    if ( isDeleting() )
    {
        m_opcount--;
        if ( m_opcount == 0 )
            emit finished();

        return;
    }

    tDebug( LOGVERBOSE ) << "DirLister::scanDir scanning:" << dir.canonicalPath();
    if ( !dir.exists() )
    {
        tDebug( LOGVERBOSE ) << "Dir no longer exists, not scanning";

        m_opcount--;
        if ( m_opcount == 0 )
            emit finished();

        return;
    }

    QFileInfoList filteredEntries;

    dir.setFilter( QDir::Files | QDir::Readable | QDir::NoDotAndDotDot );
    dir.setSorting( QDir::Name );
    filteredEntries = dir.entryInfoList();

    foreach ( const QFileInfo& di, filteredEntries )
        emit fileToScan( di );

    dir.setFilter( QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot );
    filteredEntries = dir.entryInfoList();

    foreach ( const QFileInfo& di, filteredEntries )
    {
        const QString canonical = di.canonicalFilePath();
        m_opcount++;
        QMetaObject::invokeMethod( this, "scanDir", Qt::QueuedConnection, Q_ARG( QDir, di.canonicalFilePath() ), Q_ARG( int, depth + 1 ) );
    }

    m_opcount--;
    if ( m_opcount == 0 )
    {
        tDebug() << Q_FUNC_INFO << "emitting finished";
        emit finished();
    }
}


DirListerThreadController::DirListerThreadController( QObject *parent )
    : QThread( parent )
{
    tDebug() << Q_FUNC_INFO;
}


DirListerThreadController::~DirListerThreadController()
{
    tDebug() << Q_FUNC_INFO;
}


void
DirListerThreadController::run()
{
    m_dirLister = QPointer< DirLister >( new DirLister( m_paths ) );
    connect( m_dirLister.data(), SIGNAL( fileToScan( QFileInfo ) ),
             parent(), SLOT( scanFile( QFileInfo ) ), Qt::QueuedConnection );

    // queued, so will only fire after all dirs have been scanned:
    connect( m_dirLister.data(), SIGNAL( finished() ),
             parent(), SLOT( postOps() ), Qt::QueuedConnection );

    QMetaObject::invokeMethod( m_dirLister.data(), "go", Qt::QueuedConnection );

    exec();
    if ( !m_dirLister.isNull() )
        delete m_dirLister.data();
}


MusicScanner::MusicScanner( MusicScanner::ScanMode scanMode, const QStringList& paths, quint32 bs )
    : QObject()
    , m_scanMode( scanMode )
    , m_paths( paths )
    , m_batchsize( bs )
    , m_dirListerThreadController( 0 )
{
    m_ext2mime.insert( "mp3",  TomahawkUtils::extensionToMimetype( "mp3" ) );
    m_ext2mime.insert( "ogg",  TomahawkUtils::extensionToMimetype( "ogg" ) );
    m_ext2mime.insert( "oga",  TomahawkUtils::extensionToMimetype( "oga" ) );
    m_ext2mime.insert( "mpc",  TomahawkUtils::extensionToMimetype( "mpc" ) );
    m_ext2mime.insert( "wma",  TomahawkUtils::extensionToMimetype( "wma" ) );
    m_ext2mime.insert( "aac",  TomahawkUtils::extensionToMimetype( "aac" ) );
    m_ext2mime.insert( "m4a",  TomahawkUtils::extensionToMimetype( "m4a" ) );
    m_ext2mime.insert( "mp4",  TomahawkUtils::extensionToMimetype( "mp4" ) );
    m_ext2mime.insert( "flac", TomahawkUtils::extensionToMimetype( "flac" ) );
    m_ext2mime.insert( "aiff", TomahawkUtils::extensionToMimetype( "aiff" ) );
    m_ext2mime.insert( "aif",  TomahawkUtils::extensionToMimetype( "aif" ) );
    m_ext2mime.insert( "wv",   TomahawkUtils::extensionToMimetype( "wv" ) );
}


MusicScanner::~MusicScanner()
{
    tDebug() << Q_FUNC_INFO;

    if ( m_dirListerThreadController )
    {
        m_dirListerThreadController->quit();
        m_dirListerThreadController->wait( 60000 );

        delete m_dirListerThreadController;
        m_dirListerThreadController = 0;
    }
}


void
MusicScanner::startScan()
{
    tDebug( LOGVERBOSE ) << "Loading mtimes...";
    m_scanned = m_skipped = m_cmdQueue = 0;
    m_skippedFiles.clear();

    SourceList::instance()->getLocal()->scanningProgress( m_scanned );

    // trigger the scan once we've loaded old filemtimes
    //FIXME: For multiple collection support make sure the right prefix gets passed in...or not...
    //bear in mind that simply passing in the top-level of a defined collection means it will not return items that need
    //to be removed that aren't in that root any longer -- might have to do the filtering in setMTimes based on strings
    DatabaseCommand_FileMtimes *cmd = new DatabaseCommand_FileMtimes();
    connect( cmd, SIGNAL( done( QMap< QString, QMap< unsigned int, unsigned int > > ) ),
                    SLOT( setFileMtimes( QMap< QString, QMap< unsigned int, unsigned int > > ) ) );

    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
    return;
}


void
MusicScanner::setFileMtimes( const QMap< QString, QMap< unsigned int, unsigned int > >& m )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << m.count();
    m_filemtimes = m;
    scan();
}


void
MusicScanner::scan()
{
    tDebug( LOGEXTRA ) << "Num saved file mtimes from last scan:" << m_filemtimes.size();

    connect( this, SIGNAL( batchReady( QVariantList, QVariantList ) ),
                     SLOT( commitBatch( QVariantList, QVariantList ) ), Qt::DirectConnection );

    if ( m_scanMode == MusicScanner::FileScan )
    {
        scanFilePaths();
        return;
    }

    m_dirListerThreadController = new DirListerThreadController( this );
    m_dirListerThreadController->setPaths( m_paths );
    m_dirListerThreadController->start( QThread::IdlePriority );
}


void
MusicScanner::scanFilePaths()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    foreach( QString path, m_paths )
    {
        QFileInfo fi( path );
        if ( fi.exists() && fi.isReadable() )
            scanFile( fi );
    }

    QMetaObject::invokeMethod( this, "postOps", Qt::QueuedConnection );
}


void
MusicScanner::postOps()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    if ( m_scanMode == MusicScanner::DirScan )
    {
        // any remaining stuff that wasnt emitted as a batch:
        foreach( const QString& key, m_filemtimes.keys() )
        {
            if ( !m_filemtimes[ key ].keys().isEmpty() )
                m_filesToDelete << m_filemtimes[ key ].keys().first();
        }
    }

    tDebug( LOGINFO ) << "Scanning complete, saving to database. ( deleted" << m_filesToDelete.count() << "- scanned" << m_scanned << "- skipped" << m_skipped << ")";
    tDebug( LOGEXTRA ) << "Skipped the following files (no tags / no valid audio):";
    foreach ( const QString& s, m_skippedFiles )
        tDebug( LOGEXTRA ) << s;

    if ( m_filesToDelete.length() || m_scannedfiles.length() )
    {
        SourceList::instance()->getLocal()->updateIndexWhenSynced();
        commitBatch( m_scannedfiles, m_filesToDelete );
        m_scannedfiles.clear();
        m_filesToDelete.clear();
    }

    if ( !m_cmdQueue )
        cleanup();
}


void
MusicScanner::cleanup()
{
    if ( m_dirListerThreadController )
    {
        m_dirListerThreadController->quit();
        m_dirListerThreadController->wait( 60000 );

        delete m_dirListerThreadController;
        m_dirListerThreadController = 0;
    }

    tDebug() << Q_FUNC_INFO << "emitting finished!";
    emit finished();
}


void
MusicScanner::commitBatch( const QVariantList& tracks, const QVariantList& deletethese )
{
    if ( deletethese.length() )
    {
        tDebug( LOGINFO ) << Q_FUNC_INFO << "deleting" << deletethese.length() << "tracks";
        executeCommand( QSharedPointer<DatabaseCommand>( new DatabaseCommand_DeleteFiles( deletethese, SourceList::instance()->getLocal() ) ) );
    }

    if ( tracks.length() )
    {
        tDebug( LOGINFO ) << Q_FUNC_INFO << "adding" << tracks.length() << "tracks";
        executeCommand( QSharedPointer<DatabaseCommand>( new DatabaseCommand_AddFiles( tracks, SourceList::instance()->getLocal() ) ) );
    }
}


void
MusicScanner::executeCommand( QSharedPointer< DatabaseCommand > cmd )
{
    tDebug() << Q_FUNC_INFO << m_cmdQueue;
    m_cmdQueue++;
    connect( cmd.data(), SIGNAL( finished() ), SLOT( commandFinished() ) );
    Database::instance()->enqueue( cmd );
}


void
MusicScanner::commandFinished()
{
    tDebug() << Q_FUNC_INFO << m_cmdQueue;

    if ( --m_cmdQueue == 0 )
        cleanup();
}


void
MusicScanner::scanFile( const QFileInfo& fi )
{
    if ( m_filemtimes.contains( "file://" + fi.canonicalFilePath() ) )
    {
        if ( !m_filemtimes.value( "file://" + fi.canonicalFilePath() ).values().isEmpty() &&
                fi.lastModified().toUTC().toTime_t() == m_filemtimes.value( "file://" + fi.canonicalFilePath() ).values().first() )
        {
            m_filemtimes.remove( "file://" + fi.canonicalFilePath() );
            return;
        }

        if ( !m_filemtimes.value( "file://" + fi.canonicalFilePath() ).keys().isEmpty() )
            m_filesToDelete << m_filemtimes.value( "file://" + fi.canonicalFilePath() ).keys().first();
        m_filemtimes.remove( "file://" + fi.canonicalFilePath() );
    }

    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Scanning file:" << fi.canonicalFilePath();
    QVariant m = readFile( fi );
    if ( m.toMap().isEmpty() )
        return;

    m_scannedfiles << m;
    if ( m_batchsize != 0 && (quint32)m_scannedfiles.length() >= m_batchsize )
    {
        emit batchReady( m_scannedfiles, m_filesToDelete );
        m_scannedfiles.clear();
        m_filesToDelete.clear();
    }
}


QVariant
MusicScanner::readFile( const QFileInfo& fi )
{
    const QString suffix = fi.suffix().toLower();

    if ( !m_ext2mime.contains( suffix ) )
    {
        return QVariantMap(); // invalid extension
    }

    if ( m_scanned )
        if ( m_scanned % 3 == 0 )
            SourceList::instance()->getLocal()->scanningProgress( m_scanned );
    if ( m_scanned % 100 == 0 )
        tDebug( LOGINFO ) << "Scan progress:" << m_scanned << fi.canonicalFilePath();

    #ifdef COMPLEX_TAGLIB_FILENAME
        const wchar_t *encodedName = reinterpret_cast< const wchar_t * >( fi.canonicalFilePath().utf16() );
    #else
        QByteArray fileName = QFile::encodeName( fi.canonicalFilePath() );
        const char *encodedName = fileName.constData();
    #endif

    TagLib::FileRef f( encodedName );
    if ( f.isNull() || !f.tag() )
    {
        m_skippedFiles << fi.canonicalFilePath();
        m_skipped++;
        return QVariantMap();
    }

    int bitrate = 0;
    int duration = 0;

    Tomahawk::Tag *tag = Tomahawk::Tag::fromFile( f );
    if ( f.audioProperties() )
    {
        TagLib::AudioProperties *properties = f.audioProperties();
        duration = properties->length();
        bitrate = properties->bitrate();
    }

    QString artist, album, track;
    if ( tag )
    {
        artist = tag->artist().trimmed();
        album  = tag->album().trimmed();
        track  = tag->title().trimmed();
    }
    if ( !tag || artist.isEmpty() || track.isEmpty() )
    {
        // FIXME: do some clever filename guessing
        m_skippedFiles << fi.canonicalFilePath();
        m_skipped++;
        return QVariantMap();
    }

    QString mimetype = m_ext2mime.value( suffix );
    QString url( "file://%1" );

    QVariantMap m;
    m["url"]          = url.arg( fi.canonicalFilePath() );
    m["mtime"]        = fi.lastModified().toUTC().toTime_t();
    m["size"]         = (unsigned int)fi.size();
    m["mimetype"]     = mimetype;
    m["duration"]     = duration;
    m["bitrate"]      = bitrate;
    m["artist"]       = artist;
    m["album"]        = album;
    m["track"]        = track;
    m["albumpos"]     = tag->track();
    m["year"]         = tag->year();
    m["albumartist"]  = tag->albumArtist();
    m["composer"]     = tag->composer();
    m["discnumber"]   = tag->discNumber();
    m["hash"]         = ""; // TODO

    m_scanned++;
    return m;
}

