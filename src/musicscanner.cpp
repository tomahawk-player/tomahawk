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

#include "musicscanner.h"

#include <QCoreApplication>

#include "utils/tomahawkutils.h"
#include "tomahawksettings.h"
#include "sourcelist.h"
#include "database/database.h"
#include "database/databasecommand_dirmtimes.h"
#include "database/databasecommand_filemtimes.h"
#include "database/databasecommand_collectionstats.h"
#include "database/databasecommand_addfiles.h"
#include "database/databasecommand_deletefiles.h"

#include "utils/logger.h"

using namespace Tomahawk;


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

    QFileInfoList dirs;

    dir.setFilter( QDir::Files | QDir::Readable | QDir::NoDotAndDotDot );
    dir.setSorting( QDir::Name );
    dirs = dir.entryInfoList();

    foreach ( const QFileInfo& di, dirs )
        emit fileToScan( di );

    dir.setFilter( QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot );
    dirs = dir.entryInfoList();

    foreach ( const QFileInfo& di, dirs )
    {
        const QString canonical = di.canonicalFilePath();
        m_opcount++;
        QMetaObject::invokeMethod( this, "scanDir", Qt::QueuedConnection, Q_ARG( QDir, di.canonicalFilePath() ), Q_ARG( int, depth + 1 ) );
    }

    m_opcount--;
    if ( m_opcount == 0 )
        emit finished();
}


MusicScanner::MusicScanner( const QStringList& dirs, quint32 bs )
    : QObject()
    , m_dirs( dirs )
    , m_batchsize( bs )
    , m_dirListerThreadController( 0 )
{
    m_ext2mime.insert( "mp3", TomahawkUtils::extensionToMimetype( "mp3" ) );
    m_ext2mime.insert( "ogg", TomahawkUtils::extensionToMimetype( "ogg" ) );
    m_ext2mime.insert( "mpc", TomahawkUtils::extensionToMimetype( "mpc" ) );
    m_ext2mime.insert( "wma", TomahawkUtils::extensionToMimetype( "wma" ) );
    m_ext2mime.insert( "aac", TomahawkUtils::extensionToMimetype( "aac" ) );
    m_ext2mime.insert( "m4a", TomahawkUtils::extensionToMimetype( "m4a" ) );
    m_ext2mime.insert( "mp4", TomahawkUtils::extensionToMimetype( "mp4" ) );
    m_ext2mime.insert( "flac", TomahawkUtils::extensionToMimetype( "flac" ) );
}


MusicScanner::~MusicScanner()
{
    tDebug() << Q_FUNC_INFO;

    if ( !m_dirLister.isNull() )
    {
        m_dirListerThreadController->quit();;
        m_dirListerThreadController->wait( 60000 );

        delete m_dirLister.data();
        delete m_dirListerThreadController;
        m_dirListerThreadController = 0;
    }
}


void
MusicScanner::startScan()
{
    tDebug( LOGVERBOSE ) << "Loading mtimes...";
    m_scanned = m_skipped = 0;
    m_skippedFiles.clear();

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

    m_dirListerThreadController = new QThread( this );

    m_dirLister = QWeakPointer< DirLister >( new DirLister( m_dirs ) );
    m_dirLister.data()->moveToThread( m_dirListerThreadController );

    connect( m_dirLister.data(), SIGNAL( fileToScan( QFileInfo ) ),
                                   SLOT( scanFile( QFileInfo ) ), Qt::QueuedConnection );

    // queued, so will only fire after all dirs have been scanned:
    connect( m_dirLister.data(), SIGNAL( finished() ),
                                   SLOT( listerFinished() ), Qt::QueuedConnection );

    m_dirListerThreadController->start();
    QMetaObject::invokeMethod( m_dirLister.data(), "go" );
}


void
MusicScanner::listerFinished()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    // any remaining stuff that wasnt emitted as a batch:
    foreach( const QString& key, m_filemtimes.keys() )
        m_filesToDelete << m_filemtimes[ key ].keys().first();

    commitBatch( m_scannedfiles, m_filesToDelete );

    if ( m_filesToDelete.length() || m_scannedfiles.length() )
    {
        m_scannedfiles.clear();
        m_filesToDelete.clear();

        tDebug( LOGINFO ) << "Scanning complete, saving to database. ( scanned" << m_scanned << "skipped" << m_skipped << ")";
        tDebug( LOGEXTRA ) << "Skipped the following files (no tags / no valid audio):";
        foreach ( const QString& s, m_skippedFiles )
            tDebug( LOGEXTRA ) << s;
    }
    else
        cleanup();
}


void
MusicScanner::cleanup()
{
    if ( !m_dirLister.isNull() )
    {
        m_dirListerThreadController->quit();;
        m_dirListerThreadController->wait( 60000 );

        delete m_dirLister.data();
        delete m_dirListerThreadController;
        m_dirListerThreadController = 0;
    }

    emit finished();
}


void
MusicScanner::commitBatch( const QVariantList& tracks, const QVariantList& deletethese )
{
    if ( deletethese.length() )
    {
        tDebug( LOGINFO ) << Q_FUNC_INFO << "deleting" << deletethese.length() << "tracks";
        source_ptr localsrc = SourceList::instance()->getLocal();
        executeCommand( QSharedPointer<DatabaseCommand>( new DatabaseCommand_DeleteFiles( deletethese, SourceList::instance()->getLocal() ) ) );
    }

    if ( tracks.length() )
    {
        tDebug( LOGINFO ) << Q_FUNC_INFO << "adding" << tracks.length() << "tracks";
        source_ptr localsrc = SourceList::instance()->getLocal();
        executeCommand( QSharedPointer<DatabaseCommand>( new DatabaseCommand_AddFiles( tracks, localsrc ) ) );
    }
}


void
MusicScanner::executeCommand( QSharedPointer< DatabaseCommand > cmd )
{
    m_cmdQueue << cmd.data();
    connect( cmd.data(), SIGNAL( finished() ), SLOT( commandFinished() ) );
    Database::instance()->enqueue( cmd );
}


void
MusicScanner::commandFinished()
{
    DatabaseCommand* cmd = qobject_cast< DatabaseCommand* >( sender() );
    m_cmdQueue.removeAll( cmd );

    if ( m_cmdQueue.isEmpty() )
        cleanup();
}


void
MusicScanner::scanFile( const QFileInfo& fi )
{
    tDebug() << Q_FUNC_INFO << "scanning file:" << fi.canonicalFilePath();
    if ( m_filemtimes.contains( "file://" + fi.canonicalFilePath() ) )
    {
        if ( fi.lastModified().toUTC().toTime_t() == m_filemtimes.value( "file://" + fi.canonicalFilePath() ).values().first() )
        {
            m_filemtimes.remove( "file://" + fi.canonicalFilePath() );
            return;
        }

        m_filesToDelete << m_filemtimes.value( "file://" + fi.canonicalFilePath() ).keys().first();
        m_filemtimes.remove( "file://" + fi.canonicalFilePath() );
    }

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
        m_skipped++;
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
    TagLib::Tag *tag = f.tag();
    if ( f.audioProperties() )
    {
        TagLib::AudioProperties *properties = f.audioProperties();
        duration = properties->length();
        bitrate = properties->bitrate();
    }

    QString artist = TStringToQString( tag->artist() ).trimmed();
    QString album  = TStringToQString( tag->album() ).trimmed();
    QString track  = TStringToQString( tag->title() ).trimmed();
    if ( artist.isEmpty() || track.isEmpty() )
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
    m["hash"]         = ""; // TODO

    m_scanned++;
    return m;
}
