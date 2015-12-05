/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "Album.h"
#include "DropJob.h"
#include "GlobalActionManager.h"
#include "Source.h"
#include "ViewManager.h"

#include "audio/AudioEngine.h"
#include "database/Database.h"
#include "playlist/ContextView.h"
#include "playlist/TrackView.h"
#include "playlist/PlayableModel.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/TomahawkStyle.h"
#include "utils/ImageRegistry.h"
#include "utils/Logger.h"
#include "widgets/ImageButton.h"
#include "widgets/SocialWidget.h"

#include <QDesktopServices>
#include <QDropEvent>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QNetworkReply>

const static int ALLOWED_MAX_DIVERSION = 300;

using namespace Tomahawk;


AudioControls::AudioControls( QWidget* parent )
    : BackgroundWidget( parent )
    , TomahawkUtils::DpiScaler( this )
    , ui( new Ui::AudioControls )
    , m_repeatMode( PlaylistModes::NoRepeat )
    , m_shuffled( false )
    , m_haveTiming( false )
    , m_lastSliderCheck( 0 )
    , m_parent( parent )
{
    ui->setupUi( this );
    setAutoFillBackground( false );
    BackgroundWidget::setBackgroundColor( TomahawkStyle::HEADER_BACKGROUND );
    setAcceptDrops( true );
    setFixedHeight( scaledY( 85 ) );

    QFont f = font();
    f.setPointSize( TomahawkUtils::defaultFontSize() + 3 );
    ui->trackLabel->setFont( f );

    f.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
    ui->dashLabel->setFont( f );
    ui->artistLabel->setFont( f );
    ui->artistLabel->setElideMode( Qt::ElideNone );
    ui->trackLabel->setElideMode( Qt::ElideNone );
    ui->artistLabel->setType( QueryLabel::Artist );
    ui->trackLabel->setType( QueryLabel::Track );

    QPalette queryLabelsPalette = ui->artistLabel->palette();
    queryLabelsPalette.setColor( QPalette::Foreground, Qt::white );
    ui->artistLabel->setPalette( queryLabelsPalette );
    ui->trackLabel->setPalette( queryLabelsPalette );
    ui->dashLabel->setPalette( queryLabelsPalette );

    ui->timeLabel->setPalette( queryLabelsPalette );
    ui->timeLeftLabel->setPalette( queryLabelsPalette );
    f.setPointSize( TomahawkUtils::defaultFontSize() );
    ui->timeLabel->setFont( f );
    ui->timeLeftLabel->setFont( f );

    ui->ownerButton->setFixedSize( scaled( 12, 12 ) );
    ui->prevButton->setFixedSize( scaled( 16, 16 ) );
    ui->playPauseButton->setFixedSize( scaled( 16, 16 ) );
    ui->pauseButton->setFixedSize( scaled( 16, 16 ) );
    ui->nextButton->setFixedSize( scaled( 16, 16 ) );
    ui->shuffleButton->setFixedSize( scaled( 16, 16 ) );
    ui->repeatButton->setFixedSize( scaled( 16, 16 ) );
    ui->volumeLowButton->setFixedSize( scaled( 16, 16 ) );

    ui->seekSlider->setFixedHeight( scaledY( 20 ) );
    ui->volumeSlider->setFixedHeight( scaledY( 20 ) );
    ui->timeLabel->setFixedHeight( scaledY( 20 ) );
    ui->timeLeftLabel->setFixedHeight( scaledY( 20 ) );

//    ui->prevButton->setToolTip( tr( "Previous Track" ) );
//    ui->nextButton->setToolTip( tr( "Next Track" ) );
    ui->shuffleButton->setToolTip( tr( "Shuffle" ) );
    ui->repeatButton->setToolTip( tr( "Repeat" ) );

    ui->ownerButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultResolver, TomahawkUtils::Original, ui->ownerButton->size() ) );
    ui->prevButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::PrevButton, TomahawkUtils::Original, ui->prevButton->size() ) );
    ui->prevButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::PrevButtonPressed, TomahawkUtils::Original, ui->prevButton->size() ), QIcon::Off, QIcon::Active );
    ui->playPauseButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::PlayButton, TomahawkUtils::Original, ui->playPauseButton->size() ) );
    ui->playPauseButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::PlayButtonPressed, TomahawkUtils::Original, ui->playPauseButton->size() ), QIcon::Off, QIcon::Active );
    ui->pauseButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::PauseButton, TomahawkUtils::Original, ui->pauseButton->size() ) );
    ui->pauseButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::PauseButtonPressed, TomahawkUtils::Original, ui->pauseButton->size() ), QIcon::Off, QIcon::Active );
    ui->nextButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::NextButton, TomahawkUtils::Original, ui->nextButton->size() ) );
    ui->nextButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::NextButtonPressed, TomahawkUtils::Original, ui->nextButton->size() ), QIcon::Off, QIcon::Active );
    ui->shuffleButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::ShuffleOff, TomahawkUtils::Original, ui->shuffleButton->size() ) );
    ui->shuffleButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::ShuffleOffPressed, TomahawkUtils::Original, ui->shuffleButton->size() ), QIcon::Off, QIcon::Active );
    ui->repeatButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RepeatOff, TomahawkUtils::Original, ui->repeatButton->size() ) );
    ui->repeatButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RepeatOffPressed, TomahawkUtils::Original, ui->repeatButton->size() ), QIcon::Off, QIcon::Active );
    ui->volumeLowButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::Volume, TomahawkUtils::Original, ui->volumeLowButton->size() ) );
//    ui->socialButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::Share, TomahawkUtils::Original, scaled( 20, 20 ) ) );
//    ui->loveButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::NotLoved, TomahawkUtils::Original, scaled( 20, 20 ) ) );
//    ui->loveButton->setCheckable( true );

    ui->seekSlider->setEnabled( true );
    ui->seekSlider->setAcceptWheelEvents( false );
    ui->seekSlider->setTimeLine( &m_sliderTimeLine );
    ui->volumeSlider->setRange( 0, 100 );
    ui->volumeSlider->setValue( AudioEngine::instance()->volume() );

    //ui->horizontalLayout_3->setContentsMargins( scaledX( 10 ), 0, scaledX( 8 ), 0 );

    m_phononTickCheckTimer.setSingleShot( true );

    connect( &m_phononTickCheckTimer, SIGNAL( timeout() ), SLOT( phononTickCheckTimeout() ) );
    connect( &m_sliderTimeLine,    SIGNAL( frameChanged( int ) ), ui->seekSlider, SLOT( setValue( int ) ) );

    connect( ui->seekSlider,       SIGNAL( valueChanged( int ) ), AudioEngine::instance(), SLOT( seek( int ) ) );
    connect( ui->volumeSlider,     SIGNAL( valueChanged( int ) ), AudioEngine::instance(), SLOT( setVolume( int ) ) );
    connect( ui->prevButton,       SIGNAL( clicked() ), AudioEngine::instance(), SLOT( previous() ) );
    connect( ui->playPauseButton,  SIGNAL( clicked() ), AudioEngine::instance(), SLOT( play() ) );
    connect( ui->pauseButton,      SIGNAL( clicked() ), AudioEngine::instance(), SLOT( pause() ) );
    connect( ui->nextButton,       SIGNAL( clicked() ), AudioEngine::instance(), SLOT( next() ) );
    connect( ui->volumeLowButton,  SIGNAL( clicked() ), AudioEngine::instance(), SLOT( toggleMute() ) );
//    connect( ui->volumeHighButton, SIGNAL( clicked() ), AudioEngine::instance(), SLOT( raiseVolume() ) );

    connect( ui->playPauseButton,  SIGNAL( clicked() ), SIGNAL( playPressed() ) );
    connect( ui->pauseButton,      SIGNAL( clicked() ), SIGNAL( pausePressed() ) );

    connect( ui->repeatButton,     SIGNAL( clicked() ), SLOT( onRepeatClicked() ) );
    connect( ui->shuffleButton,    SIGNAL( clicked() ), SLOT( onShuffleClicked() ) );

//    connect( ui->socialButton,     SIGNAL( clicked() ),       SLOT( onSocialButtonClicked() ) );
//    connect( ui->loveButton,       SIGNAL( clicked( bool ) ), SLOT( onLoveButtonClicked( bool ) ) );
    connect( ui->ownerButton,      SIGNAL( clicked() ),       SLOT( onOwnerButtonClicked() ) );
    connect( ui->trackLabel,       SIGNAL( clicked() ), ViewManager::instance(), SLOT( showCurrentTrack() ) );

    connect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), SLOT( onPlaybackLoading( Tomahawk::result_ptr ) ) );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ) );
    connect( AudioEngine::instance(), SIGNAL( paused() ), SLOT( onPlaybackPaused() ) );
    connect( AudioEngine::instance(), SIGNAL( resumed() ), SLOT( onPlaybackResumed() ) );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ) );
    connect( AudioEngine::instance(), SIGNAL( seeked( qint64 ) ), SLOT( onPlaybackSeeked( qint64 ) ) );
    connect( AudioEngine::instance(), SIGNAL( timerMilliSeconds( qint64 ) ), SLOT( onPlaybackTimer( qint64 ) ) );
    connect( AudioEngine::instance(), SIGNAL( trackPosition( float ) ), SLOT( onTrackPosition( float ) ) );
    connect( AudioEngine::instance(), SIGNAL( volumeChanged( int ) ), SLOT( onVolumeChanged( int ) ) );
    connect( AudioEngine::instance(), SIGNAL( mutedChanged( bool ) ), SLOT( onMutedChanged( bool ) ) );
    connect( AudioEngine::instance(), SIGNAL( controlStateChanged() ), SLOT( onControlStateChanged() ) );
    connect( AudioEngine::instance(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ), SLOT( onRepeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );
    connect( AudioEngine::instance(), SIGNAL( shuffleModeChanged( bool ) ), SLOT( onShuffleModeChanged( bool ) ) );

    connect( ViewManager::instance(), SIGNAL( viewPageDestroyed() ), SLOT( onControlStateChanged() ) );

    connect( InfoSystem::InfoSystem::instance(), SIGNAL( updatedSupportedPushTypes( Tomahawk::InfoSystem::InfoTypeSet ) ),
             SLOT( onInfoSystemPushTypesUpdated( Tomahawk::InfoSystem::InfoTypeSet ) ) );
    onInfoSystemPushTypesUpdated( InfoSystem::InfoSystem::instance()->supportedPushTypes() );

    TomahawkUtils::fixMargins( this );

    onPlaybackStopped(); // initial state
}


AudioControls::~AudioControls()
{
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
AudioControls::onMutedChanged( bool muted )
{
    ui->volumeSlider->blockSignals( true );

    if ( muted )
    {
        ui->volumeSlider->setValue( 0 );
    }
    else
    {
        ui->volumeSlider->setValue( AudioEngine::instance()->volume() );
    }

    ui->volumeSlider->blockSignals( false );
}


void
AudioControls::onControlStateChanged()
{
    tDebug() << Q_FUNC_INFO;

    if ( QThread::currentThread() != thread() )
    {
        tDebug() << Q_FUNC_INFO << "Reinvoking in correct thread!";
        QMetaObject::invokeMethod( this, "onControlStateChanged", Qt::QueuedConnection );
    }

    ui->prevButton->setEnabled( AudioEngine::instance()->canGoPrevious() );
    ui->nextButton->setEnabled( AudioEngine::instance()->canGoNext() );

    // If the ViewManager doesn't know a page for the current interface, we can't offer the jump link
//    ui->artistTrackLabel->setJumpLinkVisible( AudioEngine::instance()->currentTrackPlaylist()
//                                                && ViewManager::instance()->pageForInterface( AudioEngine::instance()->currentTrackPlaylist() ) );
}


void
AudioControls::onPlaybackStarted( const Tomahawk::result_ptr result )
{
    if ( result.isNull() )
        return;

    if ( m_currentTrack.isNull() || ( !m_currentTrack.isNull() && m_currentTrack.data()->id() != result.data()->id() ) )
        onPlaybackLoading( result );

    qint64 duration = AudioEngine::instance()->currentTrackTotalTime();

    if ( duration <= 0 )
        duration = result.data()->track()->duration() * 1000;

    ui->seekSlider->setRange( 0, duration );
    ui->seekSlider->setValue( 0 );
    ui->seekSlider->setEnabled( AudioEngine::instance()->canSeek() );

    ui->timeLabel->setText( TomahawkUtils::timeToString( 0 ) );
    ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( 0 ) );

    tLog() << Q_FUNC_INFO << duration;
    m_sliderTimeLine.setDuration( duration );
    m_sliderTimeLine.setFrameRange( 0, duration );
    m_sliderTimeLine.setCurveShape( QTimeLine::LinearCurve );
    m_sliderTimeLine.setCurrentTime( 0 );
    m_seeked = false;

    int updateRate = (double)1000 / ( (double)ui->seekSlider->contentsRect().width() / (double)( duration / 1000 ) );
    m_sliderTimeLine.setUpdateInterval( qBound( 40, updateRate, 500 ) );

    m_lastSliderCheck = 0;
    m_phononTickCheckTimer.start( 500 );
}


void
AudioControls::onPlaybackLoading( const Tomahawk::result_ptr result )
{
    if ( !m_currentTrack.isNull() )
    {
        disconnect( m_currentTrack->track().data(), SIGNAL( coverChanged() ), this, SLOT( onCoverUpdated() ) );
        disconnect( m_currentTrack->track().data(), SIGNAL( socialActionsLoaded() ), this, SLOT( onSocialActionsLoaded() ) );
    }

    m_currentTrack = result;
    connect( m_currentTrack->track().data(), SIGNAL( coverChanged() ), SLOT( onCoverUpdated() ) );
    connect( m_currentTrack->track().data(), SIGNAL( socialActionsLoaded() ), SLOT( onSocialActionsLoaded() ) );

    setUpdatesEnabled( false );
    ui->trackLabel->setResult( result );
    ui->artistLabel->setResult( result );

    const QString duration = TomahawkUtils::timeToString( result.data()->track()->duration() );
    ui->timeLabel->setFixedWidth( ui->timeLabel->fontMetrics().width( QString( duration.length(), QChar( '0' ) ) ) );
    ui->timeLabel->setText( TomahawkUtils::timeToString( 0 ) );
    ui->timeLeftLabel->setFixedWidth( ui->timeLeftLabel->fontMetrics().width( QString( duration.length() + 1, QChar( '0' ) ) ) );
    ui->timeLeftLabel->setText( "-" + duration );
    m_lastTextSecondShown = 0;

    ui->playPauseButton->setVisible( false );
    ui->pauseButton->setVisible( true );

/*    ui->loveButton->setEnabled( true );
    ui->loveButton->setVisible( true );
    ui->socialButton->setEnabled( true );
    ui->socialButton->setVisible( m_shouldShowShareAction );*/
    delete ui->horizontalLayout->takeAt( 1 );
    ui->horizontalSpacer = new QSpacerItem( 162, 8, QSizePolicy::Minimum, QSizePolicy::Minimum );
    ui->horizontalLayout->insertSpacerItem( 1, ui->horizontalSpacer );
    ui->horizontalLayout->invalidate();

    ui->dashLabel->setVisible( true );
    ui->ownerButton->setEnabled( true );
    ui->ownerButton->setVisible( true );

    ui->timeLabel->setToolTip( tr( "Time Elapsed" ) );
    ui->timeLeftLabel->setToolTip( tr( "Time Remaining" ) );
    ui->shuffleButton->setToolTip( tr( "Shuffle" ) );
    ui->repeatButton->setToolTip( tr( "Repeat" ) );
//    ui->socialButton->setToolTip( tr( "Share" ) );
//    ui->loveButton->setToolTip( tr( "Love" ) );
    ui->ownerButton->setToolTip( QString( tr( "Playing from %1" ) ).arg( result->friendlySource() ) );

    // stop the seek slider while we're still loading the track
    ui->seekSlider->setRange( 0, 0 );
    ui->seekSlider->setValue( 0 );
    ui->seekSlider->setVisible( true );
    m_sliderTimeLine.stop();

    onControlStateChanged();

    QPixmap sourceIcon = result->sourceIcon( TomahawkUtils::RoundedCorners, ui->ownerButton->size() );
    if ( !sourceIcon.isNull() )
    {
        ui->ownerButton->setPixmap( sourceIcon );
    }
    else
    {
        ui->ownerButton->clear();
        ui->ownerButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultResolver, TomahawkUtils::Original, QSize( 34, 34 ) ) );
    }

    if ( QUrl( result->linkUrl() ).isValid() || !result->resolvedByCollection().isNull() )
        ui->ownerButton->setCursor( Qt::PointingHandCursor );
    else
        ui->ownerButton->setCursor( Qt::ArrowCursor );

    setCover();
    setSocialActions();
    setUpdatesEnabled( true );
}


void
AudioControls::onCoverUpdated()
{
    Track* track = qobject_cast< Track* >( sender() );
    if ( !track || !m_currentTrack || track != m_currentTrack->track().data() )
        return;

    setCover();
}


void
AudioControls::setCover()
{
    if ( !m_currentTrack->track()->cover( QSize( 0, 0 ) ).isNull() )
    {
        QPixmap cover = m_currentTrack->track()->cover( QSize( 0, 0 ) );
        setBackground( QPixmap::fromImage( TomahawkUtils::blurred( cover.toImage(), cover.rect(), 10, false, true ) ) );
    }
    else
    {
        setBackground( QPixmap() );
    }
//        ui->coverImage->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, TomahawkUtils::Original, ui->coverImage->size() ), true );
}


void
AudioControls::onSocialActionsLoaded()
{
    Track* track = qobject_cast< Track* >( sender() );
    if ( !track || !m_currentTrack || !track->equals( m_currentTrack->track() ) )
        return;

    setSocialActions();
}


void
AudioControls::onInfoSystemPushTypesUpdated( InfoSystem::InfoTypeSet supportedTypes )
{
    if ( supportedTypes.contains( InfoSystem::InfoShareTrack ) )
    {
        m_shouldShowShareAction = true;
    }
    else
    {
        m_shouldShowShareAction = false;
    }

/*    if ( AudioEngine::instance()->state() == AudioEngine::Stopped )
        ui->socialButton->setVisible( false );
    else
        ui->socialButton->setVisible( m_shouldShowShareAction );*/
}


void
AudioControls::setSocialActions()
{
    if ( m_currentTrack->track()->loved() )
    {
/*        ui->loveButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::Loved, TomahawkUtils::Original, scaled( 20, 20 ) ) );
        ui->loveButton->setChecked( true );*/
    }
    else
    {
/*        ui->loveButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::NotLoved, TomahawkUtils::Original, scaled( 20, 20 ) ) );
        ui->loveButton->setChecked( false );*/
    }
}


void
AudioControls::onPlaybackPaused()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    ui->pauseButton->setVisible( false );
    ui->playPauseButton->setVisible( true );
    m_sliderTimeLine.setPaused( true );
}


void
AudioControls::onPlaybackResumed()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    ui->playPauseButton->setVisible( false );
    ui->pauseButton->setVisible( true );
    m_seeked = true;
    onPlaybackTimer( m_lastSliderCheck );
}


void
AudioControls::onPlaybackSeeked( qint64 msec )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    m_seeked = true;
    if ( m_haveTiming )
    {
        onPlaybackTimer( msec );
    }
}


void
AudioControls::onPlaybackStopped()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    m_currentTrack.clear();

    ui->artistLabel->clear();
    ui->trackLabel->clear();
    ui->dashLabel->setVisible( false );
    ui->timeLabel->clear();
    ui->timeLeftLabel->clear();
    ui->ownerButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultResolver, TomahawkUtils::Original, QSize( 34, 34 ) ) );

    ui->seekSlider->setVisible( false );
    m_sliderTimeLine.stop();
    m_sliderTimeLine.setCurrentTime( 0 );
    m_phononTickCheckTimer.stop();

    ui->playPauseButton->setVisible( true );
    ui->pauseButton->setVisible( false );
/*    ui->loveButton->setEnabled( false );
    ui->loveButton->setVisible( false );
    ui->socialButton->setEnabled( false );
    ui->socialButton->setVisible( false );*/
    ui->ownerButton->setEnabled( false );
    ui->ownerButton->setVisible( false );

    ui->timeLabel->setToolTip( "" );
    ui->timeLeftLabel->setToolTip( "" );
    ui->shuffleButton->setToolTip( "" );
    ui->repeatButton->setToolTip( "" );
//    ui->socialButton->setToolTip( "" );
//    ui->loveButton->setToolTip( "" );
    ui->ownerButton->setToolTip( "" );

    onControlStateChanged();

    delete ui->horizontalLayout->takeAt( 1 );
    ui->horizontalSpacer = new QSpacerItem( 162, 8, QSizePolicy::Expanding, QSizePolicy::Minimum );
    ui->horizontalLayout->insertSpacerItem( 1, ui->horizontalSpacer );
    ui->horizontalLayout->invalidate();
}


void
AudioControls::onTrackPosition( float position )
{
    if ( !m_haveTiming )
    {
        qint64 duration = AudioEngine::instance()->currentTrackTotalTime();
        ui->seekSlider->blockSignals( true );
        ui->seekSlider->setSliderPosition( position * duration );
        ui->seekSlider->blockSignals( false );
    }
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
        ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( m_currentTrack->track()->duration() - seconds ) );
        m_lastTextSecondShown = seconds;
    }

    m_phononTickCheckTimer.start( 500 );

    if ( msElapsed == 0 )
    {
        m_haveTiming = false;
        m_sliderTimeLine.stop();
        return;
    }
    m_haveTiming = true;
    if ( m_sliderTimeLine.state() != QTimeLine::Running )
        m_sliderTimeLine.resume();

    int currentTime = m_sliderTimeLine.currentTime();
    //tDebug( LOGEXTRA ) << Q_FUNC_INFO << "msElapsed =" << msElapsed << "and timer current time =" << currentTime << "and audio engine state is" << (int)AudioEngine::instance()->state();

    // First condition checks for the common case where
    // 1) the track has been started
    // 2) we haven't seeked,
    // 3) the timeline is pretty close to the actual time elapsed, within ALLOWED_MAX_DIVERSIONmsec, so no adustment needed, and
    // 4) The audio engine is actually currently running
    if ( !m_seeked
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
            // tDebug() << Q_FUNC_INFO << "Seeked";
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
            // tDebug() << Q_FUNC_INFO << "AudioEngine playing";
            m_sliderTimeLine.setPaused( true );
            m_sliderTimeLine.setCurrentTime( msElapsed );
            if ( msElapsed != m_lastSliderCheck )
                m_sliderTimeLine.resume();
        }
        // Finally, the case where the audioengine isn't playing; if the timeline is still running, pause it and catch up
        else if ( AudioEngine::instance()->state() != AudioEngine::Playing )
        {
            // tDebug() << Q_FUNC_INFO << "AudioEngine not playing";
            if ( msElapsed != currentTime || m_sliderTimeLine.state() == QTimeLine::Running )
            {
                m_sliderTimeLine.setPaused( true );
                m_sliderTimeLine.setCurrentTime( msElapsed );
            }
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
            ui->repeatButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RepeatOff, TomahawkUtils::Original, ui->repeatButton->size() ) );
            ui->repeatButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RepeatOffPressed, TomahawkUtils::Original, ui->repeatButton->size() ), QIcon::Off, QIcon::Active );
        }
        break;

        case PlaylistModes::RepeatOne:
        {
            // switch to RepeatAll
            ui->repeatButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RepeatOne, TomahawkUtils::Original, ui->repeatButton->size() ) );
            ui->repeatButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RepeatOnePressed, TomahawkUtils::Original, ui->repeatButton->size() ), QIcon::Off, QIcon::Active );
        }
        break;

        case PlaylistModes::RepeatAll:
        {
            // switch to NoRepeat
            ui->repeatButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RepeatAll, TomahawkUtils::Original, ui->repeatButton->size() ) );
            ui->repeatButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RepeatAllPressed, TomahawkUtils::Original, ui->repeatButton->size() ), QIcon::Off, QIcon::Active );
        }
        break;

        default:
            break;
    }

    onControlStateChanged();
}


void
AudioControls::onRepeatClicked()
{
    switch ( m_repeatMode )
    {
        case PlaylistModes::NoRepeat:
        {
            // switch to RepeatOne
            AudioEngine::instance()->setRepeatMode( PlaylistModes::RepeatOne );
        }
        break;

        case PlaylistModes::RepeatOne:
        {
            // switch to RepeatAll
            AudioEngine::instance()->setRepeatMode( PlaylistModes::RepeatAll );
        }
        break;

        case PlaylistModes::RepeatAll:
        {
            // switch to NoRepeat
            AudioEngine::instance()->setRepeatMode( PlaylistModes::NoRepeat );
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
        ui->shuffleButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::ShuffleOn, TomahawkUtils::Original, ui->shuffleButton->size() ) );
        ui->shuffleButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::ShuffleOnPressed, TomahawkUtils::Original, ui->shuffleButton->size() ), QIcon::Off, QIcon::Active );

        ui->repeatButton->setEnabled( false );
    }
    else
    {
        ui->shuffleButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::ShuffleOff, TomahawkUtils::Original, ui->shuffleButton->size() ) );
        ui->shuffleButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::ShuffleOffPressed, TomahawkUtils::Original, ui->shuffleButton->size() ), QIcon::Off, QIcon::Active );

        ui->repeatButton->setEnabled( true );
    }

    onControlStateChanged();
}


void
AudioControls::onShuffleClicked()
{
    AudioEngine::instance()->setShuffled( m_shuffled ^ true );
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
        ViewManager::instance()->queue()->view()->trackView()->model()->appendQueries( tracks );
    }
}


void
AudioControls::onSocialButtonClicked()
{
/*
    if ( !m_socialWidget.isNull() )
        m_socialWidget.data()->close();

    m_socialWidget = new SocialWidget( m_parent );
    QPoint socialWidgetPos = ui->socialButton->pos();
    socialWidgetPos.rx() += ui->socialButton->width() / 2;
    socialWidgetPos.ry() += scaled( 0, 6 ).height();

//    m_socialWidget.data()->setPosition( ui->metaDataArea->mapToGlobal( socialWidgetPos ) );
    m_socialWidget.data()->setQuery( m_currentTrack->toQuery() );
    m_socialWidget.data()->show();*/
}


void
AudioControls::onLoveButtonClicked( bool checked )
{
    if ( checked )
    {
//        ui->loveButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::Loved, TomahawkUtils::Original, scaled( 20, 20 ) ) );

        m_currentTrack->track()->setLoved( true );
    }
    else
    {
//        ui->loveButton->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::NotLoved, TomahawkUtils::Original, scaled( 20, 20 ) ) );

        m_currentTrack->track()->setLoved( false );
    }
}


void
AudioControls::onOwnerButtonClicked()
{
    if ( m_currentTrack->resolvedByCollection().isNull() )
    {
        QUrl url = QUrl( m_currentTrack->linkUrl() );
        if ( url.isValid() )
            QDesktopServices::openUrl( url );
    }
    else
        ViewManager::instance()->show( m_currentTrack->resolvedByCollection() );
}
