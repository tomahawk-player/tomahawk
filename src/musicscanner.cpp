#include "musicscanner.h"

#include "tomahawk/tomahawkapp.h"
#include "database.h"
#include "databasecommand_dirmtimes.h"
#include "databasecommand_addfiles.h"

using namespace Tomahawk;


MusicScanner::MusicScanner( const QString& dir, quint32 bs )
    : QThread()
    , m_dir( dir )
    , m_batchsize( bs )
{
    moveToThread( this );

    m_ext2mime.insert( "mp3",  "audio/mpeg" );

   // m_ext2mime.insert( "aac",  "audio/mp4" );
   // m_ext2mime.insert( "m4a",  "audio/mp4" );
   // m_ext2mime.insert( "mp4",  "audio/mp4" );
   // m_ext2mime.insert( "flac", "audio/flac" );

#ifndef NO_OGG
    // not compiled on windows yet
   m_ext2mime.insert( "ogg",  "application/ogg" );
#endif
}


void
MusicScanner::run()
{
    QTimer::singleShot( 0, this, SLOT( startScan() ) );
    exec(); 
}


void
MusicScanner::startScan()
{
    qDebug() << "Loading mtimes...";
    m_scanned = m_skipped = 0;
    m_skippedFiles.clear();

    // trigger the scan once we've loaded old mtimes for dirs below our path
    DatabaseCommand_DirMtimes* cmd = new DatabaseCommand_DirMtimes( m_dir );
    connect( cmd, SIGNAL( done( const QMap<QString, unsigned int>& ) ),
                SLOT( setMtimes( const QMap<QString, unsigned int>& ) ), Qt::DirectConnection );
    connect( cmd, SIGNAL( done( const QMap<QString,unsigned int>& ) ),
                    SLOT( scan() ), Qt::DirectConnection );

    TomahawkApp::instance()->database()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


void
MusicScanner::setMtimes( const QMap<QString, unsigned int>& m )
{
    m_dirmtimes = m;
}


void
MusicScanner::scan()
{
    TomahawkApp::instance()->sourcelist().getLocal()->scanningProgress( 0 );
    qDebug() << "Scanning, num saved mtimes from last scan:" << m_dirmtimes.size();

    connect( this, SIGNAL( batchReady( QVariantList ) ),
                     SLOT( commitBatch( QVariantList ) ), Qt::DirectConnection );

    DirLister* lister = new DirLister( QDir( m_dir, 0 ), m_dirmtimes );

    connect( lister, SIGNAL( fileToScan( QFileInfo ) ),
                       SLOT( scanFile( QFileInfo ) ), Qt::QueuedConnection );

    // queued, so will only fire after all dirs have been scanned:
    connect( lister, SIGNAL( finished( const QMap<QString, unsigned int>& ) ),
                SLOT( listerFinished( const QMap<QString, unsigned int>& ) ), Qt::QueuedConnection );

    connect( lister, SIGNAL( finished() ), lister, SLOT( deleteLater() ) );

    lister->start();
}


void
MusicScanner::listerFinished( const QMap<QString, unsigned int>& newmtimes )
{
    qDebug() << Q_FUNC_INFO;
    // any remaining stuff that wasnt emitted as a batch:
    if( m_scannedfiles.length() )
    {
        TomahawkApp::instance()->sourcelist().getLocal()->scanningProgress( m_scanned );
        commitBatch( m_scannedfiles );
    }

    // save mtimes, then quit thread
    DatabaseCommand_DirMtimes* cmd = new DatabaseCommand_DirMtimes( newmtimes );
    connect( cmd, SIGNAL( finished() ), SLOT( quit() ) );
    TomahawkApp::instance()->database()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

    qDebug() << "Scanning complete, saving to database. "
                "(scanned" << m_scanned << "skipped" << m_skipped << ")";

    qDebug() << "Skipped the following files (no tags / no valid audio):";
    foreach( const QString& s, m_skippedFiles )
        qDebug() << s;
}


void
MusicScanner::commitBatch( const QVariantList& tracks )
{
    if ( tracks.length() )
    {
        qDebug() << Q_FUNC_INFO << tracks.length();
        source_ptr localsrc = TomahawkApp::instance()->sourcelist().getLocal();
        TomahawkApp::instance()->database()->enqueue(
            QSharedPointer<DatabaseCommand>( new DatabaseCommand_AddFiles( tracks, localsrc ) )
        );
    }
}


void
MusicScanner::scanFile( const QFileInfo& fi )
{
    QVariant m = readFile( fi );
    if( m.toMap().isEmpty() )
        return;

    m_scannedfiles << m;
    if( m_batchsize != 0 &&
        (quint32)m_scannedfiles.length() >= m_batchsize )
    {
        qDebug() << "batchReady, size:" << m_scannedfiles.length();
        emit batchReady( m_scannedfiles );
        m_scannedfiles.clear();
    }
}


QVariant
MusicScanner::readFile( const QFileInfo& fi )
{
    if ( ! m_ext2mime.contains( fi.suffix().toLower() ) )
    {
        m_skipped++;
        return QVariantMap(); // invalid extension
    }

    if( m_scanned % 3 == 0 )
        TomahawkApp::instance()->sourcelist().getLocal()->scanningProgress( m_scanned );
    if( m_scanned % 100 == 0 )
        qDebug() << "SCAN" << m_scanned << fi.absoluteFilePath();

    TagLib::FileRef f( fi.absoluteFilePath().toUtf8().constData() );
    if ( f.isNull() || !f.tag() )
    {
        // qDebug() << "Doesn't seem to be a valid audiofile:" << fi.absoluteFilePath();
        m_skippedFiles << fi.absoluteFilePath();
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
        // qDebug() << "No tags found, skipping" << fi.absoluteFilePath();
        m_skippedFiles << fi.absoluteFilePath();
        m_skipped++;
        return QVariantMap();
    }

    QString mimetype = m_ext2mime.value( fi.suffix().toLower() );
    QString url( "file://%1" );

    QVariantMap m;
    m["url"]          = url.arg( fi.absoluteFilePath() );
    m["lastmodified"] = fi.lastModified().toUTC().toTime_t();
    m["size"]         = (unsigned int)fi.size();
    m["hash"]         = ""; // TODO
    m["mimetype"]     = mimetype;
    m["duration"]     = duration;
    m["bitrate"]      = bitrate;
    m["artist"]       = artist;
    m["album"]        = album;
    m["track"]        = track;
    m["albumpos"]     = tag->track();

    m_scanned++;
    return m;
}
