#include "tomahawktrayicon.h"

#include <QWheelEvent>

#include "tomahawk/tomahawkapp.h"
#include "audio/audioengine.h"
#include "tomahawkwindow.h"


TomahawkTrayIcon::TomahawkTrayIcon( QObject* parent )
    : QSystemTrayIcon( parent )
    , m_currentAnimationFrame( 0 )
{
    QIcon icon( RESPATH "icons/tomahawk-icon-128x128.png" );
    setIcon( icon );

    refreshToolTip();

    m_contextMenu = new QMenu();
    setContextMenu( m_contextMenu );
    
    m_playAction = m_contextMenu->addAction( tr( "Play" ) );
    m_pauseAction = m_contextMenu->addAction( tr( "Pause" ) );
    m_stopAction = m_contextMenu->addAction( tr( "Stop" ) );
    m_contextMenu->addSeparator();
    m_prevAction = m_contextMenu->addAction( tr( "Previous Track" ) );
    m_nextAction = m_contextMenu->addAction( tr( "Next Track" ) );
    m_contextMenu->addSeparator();
    m_quitAction = m_contextMenu->addAction( tr( "Quit" ) );
    
    connect( (QObject*)APP->audioEngine(), SIGNAL( loading( Tomahawk::result_ptr ) ), SLOT( setResult( Tomahawk::result_ptr ) ) );

    connect( m_playAction, SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( play() ) );
    connect( m_pauseAction, SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( pause() ) );
    connect( m_stopAction, SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( stop() ) );
    connect( m_prevAction, SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( previous() ) );
    connect( m_nextAction, SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( next() ) );
    connect( m_quitAction, SIGNAL( triggered() ), (QObject*)APP, SLOT( quit() ) );
    
    connect( &m_animationTimer, SIGNAL( timeout() ), SLOT( onAnimationTimer() ) );
    connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ), SLOT( onActivated( QSystemTrayIcon::ActivationReason ) ) );

    show();
}


TomahawkTrayIcon::~TomahawkTrayIcon()
{
    delete m_contextMenu;
}


void
TomahawkTrayIcon::setResult( const Tomahawk::result_ptr& result )
{
    m_currentTrack = result;
    refreshToolTip();
}


void
TomahawkTrayIcon::refreshToolTip()
{
    #ifdef Q_WS_MAC
    // causes issues with OS X menubar, also none
    // of the other OS X menubar icons have a tooltip
    return;
    #endif

    QString tip;
    if ( !m_currentTrack.isNull() )
    {
        tip = m_currentTrack->artist()->name() + " " + QChar( 8211 ) /*en dash*/ + " " + m_currentTrack->track();
    }
    else
    {
        tip = tr( "Currently not playing." );
    }

    #ifdef WIN32
        // Good old crappy Win32
        tip.replace( "&", "&&&" );
    #endif

    setToolTip( tip );
}


void
TomahawkTrayIcon::onAnimationTimer()
{
    /*if( m_animationPixmaps.isEmpty() )
    {
        stopIpodScrobblingAnimation();
        Q_ASSERT( !"Animation should not be started without frames being loaded" );
        return;
    }
    
    m_currentAnimationFrame++;
    if( m_currentAnimationFrame >= m_animationPixmaps.count() )
        m_currentAnimationFrame = 0;

    setIcon( m_animationPixmaps.at( m_currentAnimationFrame ) );*/
}


void
TomahawkTrayIcon::onActivated( QSystemTrayIcon::ActivationReason reason )
{
#ifdef Q_WS_MAC
    return;
#endif

    switch( reason )
    {
        case QSystemTrayIcon::Trigger:
        {
            TomahawkWindow* mainwindow = APP->mainWindow();
            if ( mainwindow->isVisible() )
            {
                mainwindow->hide();
            }
            else
            {
                mainwindow->show();
            }
        }
        break;

        default:
            break;
    }
}


bool
TomahawkTrayIcon::event( QEvent* e )
{
    // Beginning with Qt 4.3, QSystemTrayIcon supports wheel events, but only
    // on X11. Let's make it adjust the volume.
    if ( e->type() == QEvent::Wheel )
    {
        if ( ((QWheelEvent*)e)->delta() > 0 )
        {
            APP->audioEngine()->raiseVolume();
        }
        else
        {
            APP->audioEngine()->lowerVolume();
        }

        return true;
    }

    return QSystemTrayIcon::event( e );
}

