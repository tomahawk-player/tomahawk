/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "AudioControls.h"
#include "ui_AudioControls.h"

#include <QtNetwork/QNetworkReply>
#include <QtGui/QDropEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QDesktopServices>

#include "audio/AudioEngine.h"
#include "playlist/PlaylistView.h"
#include "database/Database.h"
#include "widgets/ImageButton.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "Album.h"
#include "DropJob.h"
#include "SocialWidget.h"
#include "GlobalActionManager.h"
#include "ViewManager.h"
#include "Source.h"

const static int ALLOWED_MAX_DIVERSION = 300;

using namespace Tomahawk;


AudioControls::AudioControls( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::AudioControls )
    , m_repeatMode( PlaylistModes::NoRepeat )
    , m_shuffled( false )
    , m_lastSliderCheck( 0 )
    , m_parent( parent )
{
    ui->setupUi( this );
    setAcceptDrops( true );

    QFont font( ui->artistTrackLabel->font() );
    font.setPointSize( TomahawkUtils::defaultFontSize() );

    ui->artistTrackLabel->setFont( font );
    ui->artistTrackLabel->setElideMode( Qt::ElideMiddle );
    ui->artistTrackLabel->setType( QueryLabel::ArtistAndTrack );
    ui->artistTrackLabel->setJumpLinkVisible( true );

    ui->albumLabel->setFont( font );
    ui->albumLabel->setType( QueryLabel::Album );

    ui->timeLabel->setFont( font );
    ui->timeLeftLabel->setFont( font );

    font.setPointSize( TomahawkUtils::defaultFontSize() - 2 );

    ui->prevButton->setPixmap( RESPATH "images/back-rest.png" );
    ui->prevButton->setPixmap( RESPATH "images/back-pressed.png", QIcon::Off, QIcon::Active );
    ui->playPauseButton->setPixmap( RESPATH "images/play-rest.png" );
    ui->playPauseButton->setPixmap( RESPATH "images/play-pressed.png", QIcon::Off, QIcon::Active );
    ui->pauseButton->setPixmap( RESPATH "images/pause-rest.png" );
    ui->pauseButton->setPixmap( RESPATH "images/pause-pressed.png", QIcon::Off, QIcon::Active );
    ui->nextButton->setPixmap( RESPATH "images/skip-rest.png" );
    ui->nextButton->setPixmap( RESPATH "images/skip-pressed.png", QIcon::Off, QIcon::Active );
    ui->shuffleButton->setPixmap( RESPATH "images/shuffle-off-rest.png" );
    ui->shuffleButton->setPixmap( RESPATH "images/shuffle-off-pressed.png", QIcon::Off, QIcon::Active );
    ui->repeatButton->setPixmap( RESPATH "images/repeat-off-rest.png" );
    ui->repeatButton->setPixmap( RESPATH "images/repeat-off-pressed.png", QIcon::Off, QIcon::Active );
    ui->volumeLowButton->setPixmap( RESPATH "images/volume-icon-muted.png" );
    ui->volumeHighButton->setPixmap( RESPATH "images/volume-icon-full.png" );
    ui->socialButton->setPixmap( RESPATH "images/share.png" );
    ui->loveButton->setPixmap( RESPATH "images/not-loved.png" );
    ui->loveButton->setCheckable( true );
    ui->ownerButton->setPixmap( RESPATH "images/resolver-default.png" );

    ui->socialButton->setFixedSize( QSize( 20, 20 ) );
    ui->loveButton->setFixedSize( QSize( 20, 20 ) );
    ui->ownerButton->setFixedSize( QSize( 34, 34 ) );

    ui->metaDataArea->setStyleSheet( "QWidget#metaDataArea {\nborder-width: 4px;\nborder-image: url(" RESPATH "images/now-playing-panel.png) 4 4 4 4 stretch stretch; }" );

    ui->seekSlider->setEnabled( true );
    ui->seekSlider->setTimeLine( &m_sliderTimeLine );
    ui->volumeSlider->setRange( 0, 100 );
    ui->volumeSlider->setValue( AudioEngine::instance()->volume() );

    m_phononTickCheckTimer.setSingleShot( true );

    connect( &m_phononTickCheckTimer, SIGNAL( timeout() ), SLOT( phononTickCheckTimeout() ) );
    connect( &m_sliderTimeLine,    SIGNAL( frameChanged( int ) ), ui->seekSlider, SLOT( setValue( int ) ) );

    connect( ui->seekSlider,       SIGNAL( valueChanged( int ) ), AudioEngine::instance(), SLOT( seek( int ) ) );
    connect( ui->volumeSlider,     SIGNAL( valueChanged( int ) ), AudioEngine::instance(), SLOT( setVolume( int ) ) );
    connect( ui->prevButton,       SIGNAL( clicked() ), AudioEngine::instance(), SLOT( previous() ) );
    connect( ui->playPauseButton,  SIGNAL( clicked() ), AudioEngine::instance(), SLOT( play() ) );
    connect( ui->pauseButton,      SIGNAL( clicked() ), AudioEngine::instance(), SLOT( pause() ) );
    connect( ui->nextButton,       SIGNAL( clicked() ), AudioEngine::instance(), SLOT( next() ) );
    connect( ui->volumeLowButton,  SIGNAL( clicked() ), AudioEngine::instance(), SLOT( lowerVolume() ) );
    connect( ui->volumeHighButton, SIGNAL( clicked() ), AudioEngine::instance(), SLOT( raiseVolume() ) );

    connect( ui->playPauseButton,  SIGNAL( clicked() ), SIGNAL( playPressed() ) );
    connect( ui->pauseButton,      SIGNAL( clicked() ), SIGNAL( pausePressed() ) );

    connect( ui->repeatButton,     SIGNAL( clicked() ), SLOT( onRepeatClicked() ) );
    connect( ui->shuffleButton,    SIGNAL( clicked() ), SLOT( onShuffleClicked() ) );

    connect( ui->artistTrackLabel, SIGNAL( clickedArtist() ), SLOT( onArtistClicked() ) );
    connect( ui->artistTrackLabel, SIGNAL( clickedTrack() ),  SLOT( onTrackClicked() ) );
    connect( ui->albumLabel,       SIGNAL( clickedAlbum() ),  SLOT( onAlbumClicked() ) );
    connect( ui->socialButton,     SIGNAL( clicked() ),       SLOT( onSocialButtonClicked() ) );
    connect( ui->loveButton,       SIGNAL( clicked( bool ) ), SLOT( onLoveButtonClicked( bool ) ) );
    connect( ui->ownerButton,      SIGNAL( clicked() ),       SLOT( onOwnerButtonClicked() ) );

    // <From AudioEngine>
    connect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), SLOT( onPlaybackLoading( Tomahawk::result_ptr ) ) );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ) );
    connect( AudioEngine::instance(), SIGNAL( paused() ), SLOT( onPlaybackPaused() ) );
    connect( AudioEngine::instance(), SIGNAL( resumed() ), SLOT( onPlaybackResumed() ) );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ) );
    connect( AudioEngine::instance(), SIGNAL( seeked( qint64 ) ), SLOT( onPlaybackSeeked( qint64 ) ) );
    connect( AudioEngine::instance(), SIGNAL( timerMilliSeconds( qint64 ) ), SLOT( onPlaybackTimer( qint64 ) ) );
    connect( AudioEngine::instance(), SIGNAL( volumeChanged( int ) ), SLOT( onVolumeChanged( int ) ) );

    ui->buttonAreaLayout->setSpacing( 0 );
    ui->stackedLayout->setSpacing( 0 );
    ui->stackedLayout->setContentsMargins( 0, 0, 0, 0 );
    ui->stackedLayout->setMargin( 0 );
    ui->playPauseButton->setContentsMargins( 0, 0, 0, 0 );
    ui->pauseButton->setContentsMargins( 0, 0, 0, 0 );
    ui->stackedLayout->setSizeConstraint( QLayout::SetFixedSize );

    onPlaybackStopped(); // initial state
}


AudioControls::~AudioControls()
{
    delete ui;
}


void
AudioControls::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
//            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
AudioControls::phononTickCheckTimeout()
{
    onPlaybackTimer( m_lastSliderCheck );
}


void
AudioControls::onVolumeChanged( int volume )
{
    ui->volumeSlider->blockSignals( true );
    ui->volumeSlider->setValue( volume );
    ui->volumeSlider->blockSignals( false );
}


void
AudioControls::onPlaybackStarted( const Tomahawk::result_ptr& result )
{
    if ( result.isNull() )
        return;

    if ( m_currentTrack.isNull() || ( !m_currentTrack.isNull() && m_currentTrack.data()->id() != result.data()->id() ) )
        onPlaybackLoading( result );

    qint64 duration = AudioEngine::instance()->currentTrackTotalTime();

    if ( duration == -1 )
        duration = result.data()->duration() * 1000;

    ui->seekSlider->setRange( 0, duration );
    ui->seekSlider->setValue( 0 );
    ui->seekSlider->setEnabled( AudioEngine::instance()->canSeek() );

    m_sliderTimeLine.stop();
    m_sliderTimeLine.setDuration( duration );
    m_sliderTimeLine.setFrameRange( 0, duration );
    m_sliderTimeLine.setCurveShape( QTimeLine::LinearCurve );
    m_sliderTimeLine.setCurrentTime( 0 );
    m_seeked = false;

    ui->seekSlider->setVisible( true );

    int updateRate = (double)1000 / ( (double)ui->seekSlider->contentsRect().width() / (double)( duration / 1000 ) );
    m_sliderTimeLine.setUpdateInterval( qBound( 40, updateRate, 500 ) );

    m_lastSliderCheck = 0;
    m_phononTickCheckTimer.start( 500 );
}


void
AudioControls::onPlaybackLoading( const Tomahawk::result_ptr& result )
{
    if ( !m_currentTrack.isNull() )
    {
        disconnect( m_currentTrack->toQuery().data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
        disconnect( m_currentTrack->toQuery().data(), SIGNAL( socialActionsLoaded() ), this, SLOT( onSocialActionsLoaded() ) );
    }

    m_currentTrack = result;
    connect( m_currentTrack->toQuery().data(), SIGNAL( updated() ), SLOT( onCoverUpdated() ) );
    connect( m_currentTrack->toQuery().data(), SIGNAL( socialActionsLoaded() ), SLOT( onSocialActionsLoaded() ) );

    ui->artistTrackLabel->setResult( result );
    ui->albumLabel->setResult( result );

    const QString duration = TomahawkUtils::timeToString( result.data()->duration() );
    ui->timeLabel->setFixedWidth( ui->timeLabel->fontMetrics().width( QString( duration.length(), QChar( '0' ) ) ) );
    ui->timeLabel->setText( TomahawkUtils::timeToString( 0 ) );
    ui->timeLeftLabel->setFixedWidth( ui->timeLeftLabel->fontMetrics().width( QString( duration.length() + 1, QChar( '0' ) ) ) );
    ui->timeLeftLabel->setText( "-" + duration );
    m_lastTextSecondShown = 0;

    ui->stackedLayout->setCurrentWidget( ui->pauseButton );

    ui->loveButton->setEnabled( true );
    ui->loveButton->setVisible( true );
    ui->socialButton->setEnabled( true );
    ui->socialButton->setVisible( true );
    ui->ownerButton->setEnabled( true );
    ui->ownerButton->setVisible( true );

    ui->timeLabel->setToolTip( tr( "Time Elapsed" ) );
    ui->timeLeftLabel->setToolTip( tr( "Time Remaining" ) );
    ui->shuffleButton->setToolTip( tr( "Shuffle" ) );
    ui->repeatButton->setToolTip( tr( "Repeat" ) );
    ui->socialButton->setToolTip( tr( "Share" ) );
    ui->loveButton->setToolTip( tr( "Love" ) );
    ui->ownerButton->setToolTip( QString( tr( "Playing from %1" ) ).arg( result->friendlySource() ) );
    QPixmap sourceIcon = result->sourceIcon().scaled( ui->ownerButton->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->ownerButton->setPixmap( sourceIcon );
    if ( QUrl( result->linkUrl() ).isValid() || !result->collection().isNull() )
        ui->ownerButton->setCursor( Qt::PointingHandCursor );
    else
        ui->ownerButton->setCursor( Qt::ArrowCursor );

    setCover();
    setSocialActions();
}


void
AudioControls::onCoverUpdated()
{
    Query* query = qobject_cast< Query* >( sender() );
    if ( !query || !m_currentTrack || query != m_currentTrack->toQuery().data() )
        return;

    setCover();
}


void
AudioControls::setCover()
{
    if ( !m_currentTrack->toQuery()->cover( ui->coverImage->size() ).isNull() )
    {
        QPixmap cover;
        cover = m_currentTrack->toQuery()->cover( ui->coverImage->size() );
        ui->coverImage->setPixmap( cover, false );
    }
    else
        ui->coverImage->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, TomahawkUtils::ScaledCover, ui->coverImage->size() ), true );
}


void
AudioControls::onSocialActionsLoaded()
{
    Query* query = qobject_cast< Query* >( sender() );
    if ( !query || !m_currentTrack || !query->equals( m_currentTrack->toQuery() ) )
        return;

    setSocialActions();
}


void
AudioControls::setSocialActions()
{
    if ( m_currentTrack->toQuery()->loved() )
    {
        ui->loveButton->setPixmap( RESPATH "images/loved.png" );
        ui->loveButton->setChecked( true );
    }
    else
    {
        ui->loveButton->setPixmap( RESPATH "images/not-loved.png" );
        ui->loveButton->setChecked( false );
    }
}


void
AudioControls::onPlaybackPaused()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    ui->stackedLayout->setCurrentWidget( ui->playPauseButton );
    m_sliderTimeLine.setPaused( true );
}


void
AudioControls::onPlaybackResumed()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    ui->stackedLayout->setCurrentWidget( ui->pauseButton );
}


void
AudioControls::onPlaybackSeeked( qint64 msec )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    m_seeked = true;
    onPlaybackTimer( msec );
}


void
AudioControls::onPlaybackStopped()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    m_currentTrack.clear();

    ui->artistTrackLabel->setText( "" );
    ui->albumLabel->setText( "" );
    ui->timeLabel->setText( "" );
    ui->timeLeftLabel->setText( "" );
    ui->coverImage->setPixmap( QPixmap(), false );
    ui->seekSlider->setVisible( false );
    m_sliderTimeLine.stop();
    m_sliderTimeLine.setCurrentTime( 0 );
    m_phononTickCheckTimer.stop();
    ui->ownerButton->setPixmap(  RESPATH "images/resolver-default.png" );

    ui->stackedLayout->setCurrentWidget( ui->playPauseButton );
    ui->loveButton->setEnabled( false );
    ui->loveButton->setVisible( false );
    ui->socialButton->setEnabled( false );
    ui->socialButton->setVisible( false );
    ui->ownerButton->setEnabled( false );
    ui->ownerButton->setVisible( false );

    ui->timeLabel->setToolTip( "" );
    ui->timeLeftLabel->setToolTip( "" );
    ui->shuffleButton->setToolTip( "" );
    ui->repeatButton->setToolTip( "" );
    ui->socialButton->setToolTip( "" );
    ui->loveButton->setToolTip( "" );
    ui->ownerButton->setToolTip( "" );
}


void
AudioControls::onPlaybackTimer( qint64 msElapsed )
{
    //tDebug() << Q_FUNC_INFO;

    m_phononTickCheckTimer.stop();

    if ( m_currentTrack.isNull() )
    {
        m_sliderTimeLine.stop();
        return;
    }

    const int seconds = msElapsed / 1000;
    if ( seconds != m_lastTextSecondShown )
    {
        ui->timeLabel->setText( TomahawkUtils::timeToString( seconds ) );
        ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( m_currentTrack->duration() - seconds ) );
        m_lastTextSecondShown = seconds;
    }

    m_phononTickCheckTimer.start( 500 );

    if ( msElapsed == 0 )
        return;

    int currentTime = m_sliderTimeLine.currentTime();
    //tDebug( LOGEXTRA ) << Q_FUNC_INFO << "msElapsed =" << msElapsed << "and timer current time =" << m_sliderTimeLine.currentTime();

    // First condition checks for the common case where
    // 1) the track has been started
    // 2) we haven't seeked,
    // 3) the timeline is pretty close to the actual time elapsed, within ALLOWED_MAX_DIVERSIONmsec, so no adustment needed, and
    // 4) The audio engine is actually currently running
    if ( msElapsed > 0
            && !m_seeked
            && qAbs( msElapsed - currentTime ) <= ALLOWED_MAX_DIVERSION
            && AudioEngine::instance()->state() == AudioEngine::Playing )
    {
        if ( m_sliderTimeLine.state() != QTimeLine::Running )
            m_sliderTimeLine.resume();
        m_lastSliderCheck = msElapsed;
        return;
    }
    else
    {
        //tDebug() << Q_FUNC_INFO << "Fallthrough";
        // If we're in here we're offset, so we need to do some munging around
        ui->seekSlider->blockSignals( true );

        // First handle seeks
        if ( m_seeked )
        {
            //tDebug() << Q_FUNC_INFO << "Seeked";
            m_sliderTimeLine.setPaused( true );
            m_sliderTimeLine.setCurrentTime( msElapsed );
            m_seeked = false;
            if ( AudioEngine::instance()->state() == AudioEngine::Playing )
                m_sliderTimeLine.resume();
        }
        // Next handle falling behind by too much, or getting ahead by too much (greater than allowed amount, which would have been sorted above)
        // However, a Phonon bug means that after a seek we'll actually have AudioEngine's state be Playing, when it ain't, so have to detect that
        else if ( AudioEngine::instance()->state() == AudioEngine::Playing )
        {
            //tDebug() << Q_FUNC_INFO << "AudioEngine playing";
            m_sliderTimeLine.setPaused( true );
            m_sliderTimeLine.setCurrentTime( msElapsed );
            if ( msElapsed != m_lastSliderCheck )
                m_sliderTimeLine.resume();
        }
        // Finally, the case where the audioengine isn't playing; if the timeline is still running, pause it and catch up
        else if ( AudioEngine::instance()->state() != AudioEngine::Playing )
        {
            //tDebug() << Q_FUNC_INFO << "AudioEngine not playing";
            if ( msElapsed != currentTime || m_sliderTimeLine.state() == QTimeLine::Running)
            {
                m_sliderTimeLine.setPaused( true );
                m_sliderTimeLine.setCurrentTime( msElapsed );
            }
        }
        else
        {
            tDebug() << Q_FUNC_INFO << "What to do? How could we even get here?";
        }
        m_lastSliderCheck = msElapsed;
        ui->seekSlider->blockSignals( false );
    }
}


void
AudioControls::onRepeatModeChanged( PlaylistModes::RepeatMode mode )
{
    m_repeatMode = mode;

    switch ( m_repeatMode )
    {
        case PlaylistModes::NoRepeat:
        {
            // switch to RepeatOne
            ui->repeatButton->setPixmap( RESPATH "images/repeat-off-rest.png" );
            ui->repeatButton->setPixmap( RESPATH "images/repeat-off-pressed.png", QIcon::Off, QIcon::Active );
        }
        break;

        case PlaylistModes::RepeatOne:
        {
            // switch to RepeatAll
            ui->repeatButton->setPixmap( RESPATH "images/repeat-1-on-rest.png" );
            ui->repeatButton->setPixmap( RESPATH "images/repeat-1-on-pressed.png", QIcon::Off, QIcon::Active );
        }
        break;

        case PlaylistModes::RepeatAll:
        {
            // switch to NoRepeat
            ui->repeatButton->setPixmap( RESPATH "images/repeat-all-on-rest.png" );
            ui->repeatButton->setPixmap( RESPATH "images/repeat-all-on-pressed.png", QIcon::Off, QIcon::Active );
        }
        break;

        default:
            break;
    }
}


void
AudioControls::onRepeatClicked()
{
    switch ( m_repeatMode )
    {
        case PlaylistModes::NoRepeat:
        {
            // switch to RepeatOne
            ViewManager::instance()->setRepeatMode( PlaylistModes::RepeatOne );
        }
        break;

        case PlaylistModes::RepeatOne:
        {
            // switch to RepeatAll
            ViewManager::instance()->setRepeatMode( PlaylistModes::RepeatAll );
        }
        break;

        case PlaylistModes::RepeatAll:
        {
            // switch to NoRepeat
            ViewManager::instance()->setRepeatMode( PlaylistModes::NoRepeat );
        }
        break;

        default:
            break;
    }
}


void
AudioControls::onShuffleModeChanged( bool enabled )
{
    m_shuffled = enabled;

    if ( m_shuffled )
    {
        ui->shuffleButton->setPixmap( RESPATH "images/shuffle-on-rest.png" );
        ui->shuffleButton->setPixmap( RESPATH "images/shuffle-on-pressed.png", QIcon::Off, QIcon::Active );

        ui->repeatButton->setEnabled( false );
    }
    else
    {
        ui->shuffleButton->setPixmap( RESPATH "images/shuffle-off-rest.png" );
        ui->shuffleButton->setPixmap( RESPATH "images/shuffle-off-pressed.png", QIcon::Off, QIcon::Active );

        ui->repeatButton->setEnabled( true );
    }
}


void
AudioControls::onShuffleClicked()
{
    ViewManager::instance()->setShuffled( m_shuffled ^ true );
}


void
AudioControls::onArtistClicked()
{
    ViewManager::instance()->show( m_currentTrack->artist() );
}


void
AudioControls::onAlbumClicked()
{
    ViewManager::instance()->show( m_currentTrack->album() );
}


void
AudioControls::onTrackClicked()
{
    ViewManager::instance()->show( m_currentTrack->toQuery() );
}


void
AudioControls::dragEnterEvent( QDragEnterEvent* e )
{
    if ( DropJob::acceptsMimeData( e->mimeData() ) )
        e->acceptProposedAction();
}


void
AudioControls::dragMoveEvent( QDragMoveEvent* /* e */ )
{
//     if ( GlobalActionManager::instance()->acceptsMimeData( e->mimeData() ) )
//         e->acceptProposedAction();
}


void
AudioControls::dropEvent( QDropEvent* e )
{
    tDebug() << "AudioControls got drop:" << e->mimeData()->formats();
    if ( DropJob::acceptsMimeData( e->mimeData() ) )
    {
        DropJob *dj = new DropJob();
        dj->setDropAction( DropJob::Append );
        connect( dj, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( droppedTracks( QList<Tomahawk::query_ptr> ) ) );
        dj->tracksFromMimeData( e->mimeData() );

        e->accept();
    }
}


void
AudioControls::droppedTracks( QList< query_ptr > tracks )
{
    if ( !tracks.isEmpty() )
    {
        // queue and play the first no matter what
        GlobalActionManager::instance()->handlePlayTrack( tracks.first() );
        ViewManager::instance()->queue()->model()->appendQueries( tracks );
    }
}


void
AudioControls::onSocialButtonClicked()
{
    if ( !m_socialWidget.isNull() )
        return;

    m_socialWidget = new SocialWidget( m_parent );
    m_socialWidget.data()->setPosition( m_socialWidget.data()->mapFromGlobal( QCursor::pos() ) );
    m_socialWidget.data()->setQuery( m_currentTrack->toQuery() );
    m_socialWidget.data()->show();
}


void
AudioControls::onLoveButtonClicked( bool checked )
{
    if ( checked )
    {
        ui->loveButton->setPixmap( RESPATH "images/loved.png" );

        m_currentTrack->toQuery()->setLoved( true );
    }
    else
    {
        ui->loveButton->setPixmap( RESPATH "images/not-loved.png" );

        m_currentTrack->toQuery()->setLoved( false );
    }
}


void
AudioControls::onOwnerButtonClicked()
{
    if ( m_currentTrack->collection().isNull() )
    {
        QUrl url = QUrl( m_currentTrack->linkUrl() );
        if ( url.isValid() )
            QDesktopServices::openUrl( url );
    }
    else
        ViewManager::instance()->show( m_currentTrack->collection() );
}
