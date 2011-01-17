#include "scrobbler.h"

#include <QDir>
#include <QSettings>
#include <QCryptographicHash>

#include "album.h"
#include "typedefs.h"
#include "audio/audioengine.h"
#include "tomahawksettings.h"
#include "tomahawk/tomahawkapp.h"

#include <lastfm/ws.h>
#include <lastfm/XmlQuery>


static QString
md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}


Scrobbler::Scrobbler( QObject* parent )
    : QObject( parent )
    , m_scrobbler( 0 )
    , m_reachedScrobblePoint( false )
    , m_authJob( 0 )
{
    lastfm::ws::ApiKey = "2aa1089093868876bba20b0482b9cef9";
    lastfm::ws::SharedSecret = "a7085ef81d7b46fe6ffe11c15b85902f";
    lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();
    
    m_pw = TomahawkSettings::instance()->lastFmPassword();
    
    if( TomahawkSettings::instance()->scrobblingEnabled() && !lastfm::ws::Username.isEmpty() )
    {
        createScrobbler();
    }
        
    //HACK work around a bug in liblastfm---it doesn't create its config dir, so when it
    // tries to write the track cache, it fails silently. until we have a fixed version, do this
    // code taken from Amarok (src/services/lastfm/ScrobblerAdapter.cpp)
    QString lpath = QDir::home().filePath( ".local/share/Last.fm" );
    QDir ldir = QDir( lpath );
    if( !ldir.exists() )
    {
        ldir.mkpath( lpath );
    }
    
    connect( TomahawkApp::instance(), SIGNAL( settingsChanged() ),
                                        SLOT( settingsChanged() ), Qt::QueuedConnection );

    connect( AudioEngine::instance(), SIGNAL( timerSeconds( unsigned int ) ),
                                        SLOT( engineTick( unsigned int ) ), Qt::QueuedConnection );
}


Scrobbler::~Scrobbler()
{
    delete m_scrobbler;
}


void 
Scrobbler::trackStarted( const Tomahawk::result_ptr& track )
{
    Q_ASSERT( QThread::currentThread() == thread() );
//    qDebug() << Q_FUNC_INFO;

    if( !m_scrobbler )
        return;

    if( m_reachedScrobblePoint )
    {
        m_reachedScrobblePoint = false;
        scrobble();
    }

    m_track = lastfm::MutableTrack();
    m_track.stamp();

    m_track.setTitle( track->track() );
    m_track.setArtist( track->artist()->name() );
    m_track.setAlbum( track->album()->name() );
    m_track.setDuration( track->duration() );
    m_track.setSource( lastfm::Track::Player );

    m_scrobbler->nowPlaying( m_track );
    m_scrobblePoint = ScrobblePoint( m_track.duration() / 2 );
}


void 
Scrobbler::trackPaused()
{
    Q_ASSERT( QThread::currentThread() == thread() );
}


void 
Scrobbler::trackResumed()
{
    Q_ASSERT( QThread::currentThread() == thread() );
}


void 
Scrobbler::trackStopped()
{
    Q_ASSERT( QThread::currentThread() == thread() );

    if( m_reachedScrobblePoint )
    {
        m_reachedScrobblePoint = false;
        scrobble();
    }
}


void
Scrobbler::engineTick( unsigned int secondsElapsed )
{
    if ( secondsElapsed > m_scrobblePoint )
        m_reachedScrobblePoint = true;
}


void
Scrobbler::scrobble()
{
    Q_ASSERT( QThread::currentThread() == thread() );

    qDebug() << Q_FUNC_INFO << m_track.toString();
    m_scrobbler->cache( m_track );
    m_scrobbler->submit();
}


void
Scrobbler::settingsChanged()
{
    if( !m_scrobbler && TomahawkSettings::instance()->scrobblingEnabled() )
    { // can simply create the scrobbler
        lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();
        m_pw = TomahawkSettings::instance()->lastFmPassword();

        createScrobbler();
    }
    else if( m_scrobbler && !TomahawkSettings::instance()->scrobblingEnabled() )
    {
        delete m_scrobbler;
        m_scrobbler = 0;
    }
    else if( TomahawkSettings::instance()->lastFmUsername() != lastfm::ws::Username ||
               TomahawkSettings::instance()->lastFmPassword() != m_pw )
    {
        lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();
        // credentials have changed, have to re-create scrobbler for them to take effect
        if( m_scrobbler )
            delete m_scrobbler;

        createScrobbler();
    }
}


void
Scrobbler::onAuthenticated()
{
    if( !m_authJob ) 
    {
        qDebug() << Q_FUNC_INFO << "Help! No longer got a last.fm auth job!";
        return;
    }
    
    if( m_authJob->error() == QNetworkReply::NoError )
    {
        lastfm::XmlQuery lfm = lastfm::XmlQuery( m_authJob->readAll() );

        if( lfm.children( "error" ).size() > 0 )
        {
            qDebug() << "Error from authenticating with Last.fm service:" << lfm.text();
            TomahawkSettings::instance()->setLastFmSessionKey( QByteArray() );
            
        }
        else
        {
            lastfm::ws::SessionKey = lfm[ "session" ][ "key" ].text();

            TomahawkSettings::instance()->setLastFmSessionKey( lastfm::ws::SessionKey.toLatin1() );

            if( TomahawkSettings::instance()->scrobblingEnabled() )
                m_scrobbler = new lastfm::Audioscrobbler( "tst" );
        }
    }
    else
    {
        qDebug() << "Got error in Last.fm authentication job:" << m_authJob->errorString();
    }
    
    m_authJob->deleteLater();
}


void
Scrobbler::createScrobbler()
{
    if( TomahawkSettings::instance()->lastFmSessionKey().isEmpty() ) // no session key, so get one
    {
        QString authToken = md5( ( lastfm::ws::Username.toLower() + md5( m_pw.toUtf8() ) ).toUtf8() );
        
        QMap<QString, QString> query;
        query[ "method" ] = "auth.getMobileSession";
        query[ "username" ] = lastfm::ws::Username;
        query[ "authToken" ] = authToken;
        m_authJob = lastfm::ws::post( query );
        
        connect( m_authJob, SIGNAL( finished() ), SLOT( onAuthenticated() ) );
    }
    else
    {
        lastfm::ws::SessionKey = TomahawkSettings::instance()->lastFmSessionKey();
        
        m_scrobbler = new lastfm::Audioscrobbler( "tst" );
        m_scrobbler->moveToThread( thread() );
    }
}
