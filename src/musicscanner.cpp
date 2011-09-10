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
    tLog() << Q_FUNC_INFO << "Recursive? :" << (m_recursive ? "true" : "false");
    tLog() << Q_FUNC_INFO << "Manual full? :" << (m_manualFull ? "true" : "false");
    if( !m_recursive )
    {
        foreach( QString dir, m_dirs )
        {
            if( m_dirmtimes.contains( dir ) )
            {
                tDebug( LOGEXTRA ) << "Removing" << dir << "from m_dirmtimes because it's specifically requested";
                m_dirmtimes.remove( dir );
            }
            QStringList filtered = QStringList( m_dirmtimes.keys() ).filter( dir );
            foreach( QString filteredDir, filtered )
            {
                if( !QDir( filteredDir ).exists() )
                {
                    tDebug( LOGEXTRA ) << "Removing" << filteredDir << "from m_dirmtimes because it does not exist";
                    m_dirmtimes.remove( filteredDir );
                }
            }
        }
        m_newdirmtimes = m_dirmtimes;
    }

    foreach( QString dir, m_dirs )
    {
        m_opcount++;
        QMetaObject::invokeMethod( this, "scanDir", Qt::QueuedConnection, Q_ARG( QDir, QDir( dir, 0 ) ), Q_ARG( int, 0 ), Q_ARG( DirLister::Mode, ( m_recursive ? DirLister::Recursive : DirLister::NonRecursive ) ) );
    }
}


void
DirLister::scanDir( QDir dir, int depth, DirLister::Mode mode )
{
    if ( isDeleting() )
    {
        m_opcount--;
        if ( m_opcount == 0 )
            emit finished( m_newdirmtimes );

        return;
    }

    tDebug( LOGVERBOSE ) << "DirLister::scanDir scanning:" << dir.canonicalPath() << "with mode" << mode;
    if( !dir.exists() )
    {
        tDebug( LOGVERBOSE ) << "Dir no longer exists, not scanning";

        m_opcount--;
        if ( m_opcount == 0 )
            emit finished( m_newdirmtimes );

        return;
    }

    QFileInfoList dirs;
    const uint mtime = QFileInfo( dir.canonicalPath() ).lastModified().toUTC().toTime_t();
    m_newdirmtimes.insert( dir.canonicalPath(), mtime );

    if ( !m_manualFull && m_mode == TomahawkSettings::Dirs && m_dirmtimes.contains( dir.canonicalPath() ) && mtime == m_dirmtimes.value( dir.canonicalPath() ) )
    {
        // dont scan this dir, unchanged since last time.
    }
    else
    {
        if( m_manualFull ||
                ( m_mode == TomahawkSettings::Dirs
                    && ( m_dirmtimes.contains( dir.canonicalPath() ) || !m_recursive )
                    && mtime != m_dirmtimes.value( dir.canonicalPath() ) ) )
        {
            tDebug( LOGINFO ) << "Deleting database file entries from" << dir.canonicalPath();
            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( new DatabaseCommand_DeleteFiles( dir, SourceList::instance()->getLocal() ) ) );
        }

        dir.setFilter( QDir::Files | QDir::Readable | QDir::NoDotAndDotDot );
        dir.setSorting( QDir::Name );
        dirs = dir.entryInfoList();
        foreach( const QFileInfo& di, dirs )
            emit fileToScan( di );
    }
    dir.setFilter( QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot );
    dirs = dir.entryInfoList();

    foreach( const QFileInfo& di, dirs )
    {
        const QString canonical = di.canonicalFilePath();
        const bool haveDi = m_dirmtimes.contains( canonical );
        if( !m_newdirmtimes.contains( canonical ) && ( mode == DirLister::Recursive || !haveDi ) ) {
            m_opcount++;
            QMetaObject::invokeMethod( this, "scanDir", Qt::QueuedConnection, Q_ARG( QDir, di.canonicalFilePath() ), Q_ARG( int, depth + 1 ), Q_ARG( DirLister::Mode, DirLister::Recursive ) );
        }
    }

    m_opcount--;
    if ( m_opcount == 0 )
        emit finished( m_newdirmtimes );
}


MusicScanner::MusicScanner( const QStringList& dirs, TomahawkSettings::ScannerMode mode, bool manualFull, bool recursive, quint32 bs )
    : QObject()
    , m_dirs( dirs )
    , m_mode( manualFull ? TomahawkSettings::Dirs : mode )
    , m_manualFull( manualFull )
    , m_recursive( recursive )
    , m_batchsize( bs )
    , m_dirListerThreadController( 0 )
{
    m_ext2mime.insert( "mp3", TomahawkUtils::extensionToMimetype( "mp3" ) );
    m_ext2mime.insert( "ogg", TomahawkUtils::extensionToMimetype( "ogg" ) );
    m_ext2mime.insert( "flac", TomahawkUtils::extensionToMimetype( "flac" ) );
    m_ext2mime.insert( "mpc", TomahawkUtils::extensionToMimetype( "mpc" ) );
    m_ext2mime.insert( "wma", TomahawkUtils::extensionToMimetype( "wma" ) );
    m_ext2mime.insert( "aac", TomahawkUtils::extensionToMimetype( "aac" ) );
    m_ext2mime.insert( "m4a", TomahawkUtils::extensionToMimetype( "m4a" ) );
    m_ext2mime.insert( "mp4", TomahawkUtils::extensionToMimetype( "mp4" ) );
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

    // trigger the scan once we've loaded old mtimes for dirs below our path
    //FIXME: For multiple collection support make sure the right prefix gets passed in...or not...
    //bear in mind that simply passing in the top-level of a defined collection means it will not return items that need
    //to be removed that aren't in that root any longer -- might have to do the filtering in setMTimes based on strings
    DatabaseCommand_DirMtimes *cmd = new DatabaseCommand_DirMtimes();
    connect( cmd, SIGNAL( done( QMap< QString, unsigned int > ) ),
                    SLOT( setDirMtimes( QMap< QString, unsigned int > ) ) );

    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
}


void
MusicScanner::setDirMtimes( const QMap< QString, unsigned int >& m )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << m.count();
    m_dirmtimes = m;
    if ( m_mode == TomahawkSettings::Files )
    {
        DatabaseCommand_FileMtimes *cmd = new DatabaseCommand_FileMtimes();
        connect( cmd, SIGNAL( done( QMap< QString, QMap< unsigned int, unsigned int > > ) ),
                    SLOT( setFileMtimes( QMap< QString, QMap< unsigned int, unsigned int > > ) ) );

        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
        return;
    }
    scan();
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
    tDebug( LOGEXTRA ) << "Scanning, num saved dir mtimes from last scan:" << m_dirmtimes.size();
    if ( m_mode == TomahawkSettings::Files )
        tDebug( LOGEXTRA ) << "Num saved file mtimes from last scan:" << m_filemtimes.size();

    connect( this, SIGNAL( batchReady( QVariantList, QVariantList ) ),
                     SLOT( commitBatch( QVariantList, QVariantList ) ), Qt::DirectConnection );

    m_dirListerThreadController = new QThread( this );

    m_dirLister = QWeakPointer< DirLister >( new DirLister( m_dirs, m_dirmtimes, m_mode, m_manualFull, m_recursive ) );
    m_dirLister.data()->moveToThread( m_dirListerThreadController );

    connect( m_dirLister.data(), SIGNAL( fileToScan( QFileInfo ) ),
                            SLOT( scanFile( QFileInfo ) ), Qt::QueuedConnection );

    // queued, so will only fire after all dirs have been scanned:
    connect( m_dirLister.data(), SIGNAL( finished( QMap< QString, unsigned int > ) ),
                            SLOT( listerFinished( QMap< QString, unsigned int > ) ), Qt::QueuedConnection );

    m_dirListerThreadController->start();
    QMetaObject::invokeMethod( m_dirLister.data(), "go" );
}


void
MusicScanner::listerFinished( const QMap<QString, unsigned int>& newmtimes  )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    // any remaining stuff that wasnt emitted as a batch:
    foreach( QString key, m_filemtimes.keys() )
    {
        m_filesToDelete << m_filemtimes[ key ].keys().first();
    }
    commitBatch( m_scannedfiles, m_filesToDelete );

    // remove obsolete / stale files
    foreach ( const QString& path, m_dirmtimes.keys() )
    {
        if ( !newmtimes.keys().contains( path ) )
        {
            tDebug( LOGINFO ) << "Removing stale dir:" << path;
            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( new DatabaseCommand_DeleteFiles( path, SourceList::instance()->getLocal() ) ) );
        }
    }

    tDebug( LOGINFO ) << "Scanning complete, saving to database. "
                         "( scanned" << m_scanned << "skipped" << m_skipped << ")";

    tDebug( LOGEXTRA ) << "Skipped the following files (no tags / no valid audio):";
    foreach( const QString& s, m_skippedFiles )
        tDebug( LOGEXTRA ) << s;

    // save mtimes, then quit thread
    {
        DatabaseCommand_DirMtimes* cmd = new DatabaseCommand_DirMtimes( newmtimes );
        connect( cmd, SIGNAL( finished() ), SIGNAL( finished() ) );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
    }

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
MusicScanner::commitBatch( const QVariantList& tracks, const QVariantList& deletethese )
{
    if ( deletethese.length() )
    {
        tDebug( LOGINFO ) << Q_FUNC_INFO << "deleting" << deletethese.length() << "tracks";
        source_ptr localsrc = SourceList::instance()->getLocal();
        Database::instance()->enqueue(
            QSharedPointer<DatabaseCommand>( new DatabaseCommand_DeleteFiles( deletethese, SourceList::instance()->getLocal() ) )
        );
    }
    if ( tracks.length() )
    {
        tDebug( LOGINFO ) << Q_FUNC_INFO << "adding" << tracks.length() << "tracks";
        source_ptr localsrc = SourceList::instance()->getLocal();
        Database::instance()->enqueue(
            QSharedPointer<DatabaseCommand>( new DatabaseCommand_AddFiles( tracks, localsrc ) )
        );
    }
}


void
MusicScanner::scanFile( const QFileInfo& fi )
{
    if ( m_mode == TomahawkSettings::Files && m_filemtimes.contains( "file://" + fi.canonicalFilePath() ) )
    {
        if ( fi.lastModified().toUTC().toTime_t() == m_filemtimes.value( "file://" + fi.canonicalFilePath() ).values().first() )
        {
            m_filemtimes.remove( "file://" + fi.canonicalFilePath() );
            return;
        }

        m_filesToDelete << m_filemtimes.value( "file://" + fi.canonicalFilePath() ).keys().first();
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

    if ( ! m_ext2mime.contains( suffix ) )
    {
        m_skipped++;
        return QVariantMap(); // invalid extension
    }

    if ( m_scanned )
        if( m_scanned % 3 == 0 )
            SourceList::instance()->getLocal()->scanningProgress( m_scanned );
    if( m_scanned % 100 == 0 )
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
