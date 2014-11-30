/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "database/Database.h"
#include "database/DatabaseCommand_DirMtimes.h"
#include "database/DatabaseCommand_FileMTimes.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "database/DatabaseCommand_AddFiles.h"
#include "database/DatabaseCommand_DeleteFiles.h"
#include "taghandlers/tag.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"

#include "PlaylistEntry.h"
#include "SourceList.h"
#include "TomahawkSettings.h"

#include "config.h"

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>

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
    if ( !dir.exists() || m_processedDirs.contains( dir.canonicalPath() ) )
    {
        tDebug( LOGVERBOSE ) << "Dir no longer exists or already scanned, ignoring";

        m_opcount--;
        if ( m_opcount == 0 )
            emit finished();

        return;
    }

    m_processedDirs << dir.canonicalPath();

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
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
}


DirListerThreadController::~DirListerThreadController()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
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


MusicScanner::MusicScanner( MusicScanner::ScanMode scanMode, const QStringList& paths, libvlc_instance_t* pVlcInstance, quint32 bs )
    : QObject()
    , m_scanMode( scanMode )
    , m_paths( paths )
    , m_scanned( 0 )
    , m_dryRun( false )
    , m_verbose( false )
    , m_cmdQueue( 0 )
    , m_batchsize( bs )
    , m_vlcInstance( pVlcInstance )
    , m_dirListerThreadController( 0 )
{
    // We keep a strong reference to the libvlc instance here
    libvlc_retain( m_vlcInstance );
}


MusicScanner::~MusicScanner()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    if ( m_dirListerThreadController )
    {
        m_dirListerThreadController->quit();
        m_dirListerThreadController->wait( 60000 );

        delete m_dirListerThreadController;
        m_dirListerThreadController = 0;
    }

    libvlc_release( m_vlcInstance );
}


void
MusicScanner::setDryRun( bool _dryRun )
{
    m_dryRun = _dryRun;
}


bool
MusicScanner::dryRun()
{
    return m_dryRun;
}


void
MusicScanner::setVerbose( bool _verbose )
{
    m_verbose = _verbose;
}


bool
MusicScanner::verbose()
{
    return m_verbose;
}


void
MusicScanner::startScan()
{
    tDebug( LOGVERBOSE ) << "Loading mtimes...";
    m_scanned = m_skipped = m_cmdQueue = 0;
    m_skippedFiles.clear();

    emit progress( m_scanned );

    // trigger the scan once we've loaded old filemtimes
    //FIXME: For multiple collection support make sure the right prefix gets passed in...or not...
    //bear in mind that simply passing in the top-level of a defined collection means it will not return items that need
    //to be removed that aren't in that root any longer -- might have to do the filtering in setMTimes based on strings
    DatabaseCommand_FileMtimes *cmd = new DatabaseCommand_FileMtimes();
    connect( cmd, SIGNAL( done( QMap< QString, QMap< unsigned int, unsigned int > > ) ),
                    SLOT( setFileMtimes( QMap< QString, QMap< unsigned int, unsigned int > > ) ) );

    Database::instance()->enqueue( dbcmd_ptr( cmd ) );
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
    tDebug( LOGVERBOSE ) << "Num saved file mtimes from last scan:" << m_filemtimes.size();

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
        if ( !m_dryRun )
        {
            SourceList::instance()->getLocal()->updateIndexWhenSynced();
            commitBatch( m_scannedfiles, m_filesToDelete );
        }
        m_scannedfiles.clear();
        m_filesToDelete.clear();
    }

    m_processedFiles.clear();

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
        executeCommand( dbcmd_ptr( new DatabaseCommand_DeleteFiles( deletethese, SourceList::instance()->getLocal() ) ) );
    }

    if ( tracks.length() )
    {
        tDebug( LOGINFO ) << Q_FUNC_INFO << "adding" << tracks.length() << "tracks";
        executeCommand( dbcmd_ptr( new DatabaseCommand_AddFiles( tracks, SourceList::instance()->getLocal() ) ) );
    }
}


void
MusicScanner::executeCommand( dbcmd_ptr cmd )
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
    // Don't process a single file twice, this might happen if you add a subfolder of another collection folder to your collection
    if ( m_processedFiles.contains( fi.canonicalFilePath() ) )
        return;
    else
        m_processedFiles << fi.canonicalFilePath();


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

QString
vlcMetaToQString( libvlc_media_t* media, libvlc_meta_t meta )
{
    char *str = libvlc_media_get_meta(media, meta);
    if ( str )
    {
        QString qstr = QString::fromUtf8( str );
        libvlc_free( str );
        return qstr;
    }
    return QString();
}

QVariant
MusicScanner::readTags( const QFileInfo& fi, libvlc_instance_t* vlcInstance )
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Parsing tags for file:" << fi.absoluteFilePath();

    const QString suffix = fi.suffix().toLower();
    const QString mimetype = TomahawkUtils::extensionToMimetype( suffix );
    const QString url( "file://%1" );

    QVariantMap m;
    m["url"]          = url.arg( fi.canonicalFilePath() );
    m["mtime"]        = fi.lastModified().toUTC().toTime_t();
    m["size"]         = (unsigned int)fi.size();
    m["mimetype"]     = mimetype;

#ifdef HAVE_VLC_ALBUMARTIST
    libvlc_media_t* media = libvlc_media_new_path( vlcInstance, fi.canonicalFilePath().toUtf8().constData() );
    libvlc_media_parse( media );

    m["duration"] = (qlonglong)( libvlc_media_get_duration( media ) / 1000 );
    libvlc_media_track_t **tracks;
    uint track_count;
    if ( ( track_count = libvlc_media_tracks_get( media, &tracks ) ) > 0 )
    {
        // FIXME: Should we count all or just one track?
        //        Or only the first audio tracks?
        // For most audio files we only have a single audio track.
        m["bitrate"] = tracks[0]->i_bitrate / 1000;
        libvlc_media_tracks_release(tracks, track_count);
    }
    m["artist"] = vlcMetaToQString( media, libvlc_meta_Artist );
    m["album"] = vlcMetaToQString( media, libvlc_meta_Album );
    m["albumartist"] = vlcMetaToQString( media, libvlc_meta_AlbumArtist );
    m["track"] = vlcMetaToQString( media, libvlc_meta_Title );
    m["albumpos"] = vlcMetaToQString( media, libvlc_meta_TrackNumber );
    m["year"] = vlcMetaToQString( media, libvlc_meta_Date );
    // m["composer"]     = tag->composer();
    // m["discnumber"]   = tag->discNumber();

    libvlc_media_release( media );
#else
    Q_UNUSED( vlcInstance );

    if ( !TomahawkUtils::supportedExtensions().contains( suffix ) )
        return QVariantMap(); // invalid extension

    #ifdef Q_OS_WIN
        const wchar_t* encodedName = fi.canonicalFilePath().toStdWString().c_str();
    #else
        QByteArray fileName = QFile::encodeName( fi.canonicalFilePath() );
        const char* encodedName = fileName.constData();
    #endif

    TagLib::FileRef f( encodedName );
    if ( f.isNull() || !f.tag() )
        return QVariantMap();

    int bitrate = 0;
    int duration = 0;
    QSharedPointer<Tag> tag( Tag::fromFile( f ) );
    if ( !tag )
        return QVariantMap();

    if ( f.audioProperties() )
    {
        TagLib::AudioProperties *properties = f.audioProperties();
        duration = properties->length();
        bitrate = properties->bitrate();
    }

    const QString artist = tag->artist().trimmed();
    const QString album  = tag->album().trimmed();
    const QString track  = tag->title().trimmed();
    if ( artist.isEmpty() || track.isEmpty() )
        return QVariantMap();

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
#endif

    return m;
}


QVariant
MusicScanner::readFile( const QFileInfo& fi )
{
    const QVariant m = readTags( fi, m_vlcInstance );

    if ( m_scanned )
        if ( m_scanned % 3 == 0 )
            emit progress( m_scanned );

    if ( m_scanned % 100 == 0 || m_verbose )
      tDebug( LOGINFO ) << "Scanning file:" << m_scanned << fi.canonicalFilePath();

    if ( m.toMap().isEmpty() )
    {
        m_skippedFiles << fi.canonicalFilePath();
        m_skipped++;
    }
    else
    {
        m_scanned++;
    }

    return m;
}

