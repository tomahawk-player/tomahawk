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

#include "audiocontrols.h"
#include "ui_audiocontrols.h"

#include <QtNetwork/QNetworkReply>
#include <QtGui/QDropEvent>
#include <QtGui/QMouseEvent>

#include "audio/audioengine.h"
#include "playlist/playlistview.h"
#include "database/database.h"
#include "database/databasecommand_socialaction.h"
#include "widgets/imagebutton.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"
#include "album.h"
#include "dropjob.h"
#include "globalactionmanager.h"
#include "viewmanager.h"

using namespace Tomahawk;

static QString s_acInfoIdentifier = QString( "AUDIOCONTROLS" );


AudioControls::AudioControls( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::AudioControls )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
    ui->setupUi( this );
    setAcceptDrops( true );

    QFont font( ui->artistTrackLabel->font() );
    font.setPixelSize( 12 );

#ifdef Q_WS_MAC
    font.setPixelSize( font.pixelSize() - 2 );
#endif

    ui->artistTrackLabel->setFont( font );
    ui->artistTrackLabel->setElideMode( Qt::ElideMiddle );
    ui->artistTrackLabel->setType( QueryLabel::ArtistAndTrack );

    ui->albumLabel->setFont( font );
    ui->albumLabel->setType( QueryLabel::Album );

    ui->timeLabel->setFont( font );
    ui->timeLeftLabel->setFont( font );

    font.setPixelSize( 9 );
    ui->ownerLabel->setFont( font );

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
    ui->loveButton->setPixmap( RESPATH "images/not-loved.png" );
    ui->loveButton->setCheckable( true );

#ifdef Q_WS_MAC
    ui->ownerLabel->setForegroundRole( QPalette::Text );
#else
    ui->ownerLabel->setForegroundRole( QPalette::Dark );
#endif
    ui->metaDataArea->setStyleSheet( "QWidget#metaDataArea {\nborder-width: 4px;\nborder-image: url(" RESPATH "images/now-playing-panel.png) 4 4 4 4 stretch stretch; }" );

    ui->seekSlider->setEnabled( true );
    ui->volumeSlider->setRange( 0, 100 );
    ui->volumeSlider->setValue( AudioEngine::instance()->volume() );

    m_sliderTimeLine.setCurveShape( QTimeLine::LinearCurve );
    ui->seekSlider->setTimeLine( &m_sliderTimeLine );

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
    connect( ui->artistTrackLabel, SIGNAL( clickedTrack() ), SLOT( onTrackClicked() ) );
    connect( ui->albumLabel,       SIGNAL( clickedAlbum() ), SLOT( onAlbumClicked() ) );
    connect( ui->loveButton,       SIGNAL( clicked( bool ) ), SLOT( onLoveButtonClicked( bool ) ) );

    // <From AudioEngine>
    connect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), SLOT( onPlaybackLoading( Tomahawk::result_ptr ) ) );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ) );
    connect( AudioEngine::instance(), SIGNAL( paused() ), SLOT( onPlaybackPaused() ) );
    connect( AudioEngine::instance(), SIGNAL( resumed() ), SLOT( onPlaybackResumed() ) );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ) );
    connect( AudioEngine::instance(), SIGNAL( seeked( qint64 ) ), SLOT( onPlaybackSeeked( qint64 ) ) );
    connect( AudioEngine::instance(), SIGNAL( timerMilliSeconds( qint64 ) ), SLOT( onPlaybackTimer( qint64 ) ) );
    connect( AudioEngine::instance(), SIGNAL( volumeChanged( int ) ), SLOT( onVolumeChanged( int ) ) );

    m_defaultCover = QPixmap( RESPATH "images/no-album-no-case.png" )
                     .scaled( ui->coverImage->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
        SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
        SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

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
AudioControls::onVolumeChanged( int volume )
{
    ui->volumeSlider->blockSignals( true );
    ui->volumeSlider->setValue( volume );
    ui->volumeSlider->blockSignals( false );
}


void
AudioControls::onPlaybackStarted( const Tomahawk::result_ptr& result )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( result.isNull() )
        return;

    if ( m_currentTrack.isNull() || ( !m_currentTrack.isNull() && m_currentTrack.data()->id() != result.data()->id() ) )
        onPlaybackLoading( result );

    qint64 duration = AudioEngine::instance()->currentTrackTotalTime();

    if ( duration == -1 )
        duration = result.data()->duration() * 1000;

    ui->seekSlider->setRange( 0, duration );
    ui->seekSlider->setValue( 0 );

    m_sliderTimeLine.stop();
    m_sliderTimeLine.setDuration( duration );
    m_sliderTimeLine.setFrameRange( 0, duration );
    m_sliderTimeLine.setCurrentTime( 0 );
    m_seekMsecs = -1;

    ui->seekSlider->setVisible( true );

    m_noTimeChange = false;

    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    trackInfo["artist"] = result->artist()->name();
    trackInfo["album"] = result->album()->name();

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_acInfoIdentifier;
    requestData.type = Tomahawk::InfoSystem::InfoAlbumCoverArt;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    requestData.customData = QVariantMap();

    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
}


void
AudioControls::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != s_acInfoIdentifier || requestData.type != Tomahawk::InfoSystem::InfoAlbumCoverArt )
    {
        return;
    }

    if ( m_currentTrack.isNull() )
    {
        tLog() << "Current track is null when trying to apply fetched cover art";
        return;
    }

    if ( !output.canConvert< QVariantMap >() )
    {
        tDebug( LOGINFO ) << "Cannot convert fetched art from a QByteArray";
        return;
    }

    QVariantMap returnedData = output.value< QVariantMap >();
    const QByteArray ba = returnedData["imgbytes"].toByteArray();
    if ( ba.length() )
    {
        QPixmap pm;
        pm.loadFromData( ba );

        if ( pm.isNull() )
            ui->coverImage->setPixmap( m_defaultCover );
        else
            ui->coverImage->setPixmap( pm.scaled( ui->coverImage->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
    }
}


void
AudioControls::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
AudioControls::onPlaybackLoading( const Tomahawk::result_ptr& result )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    m_currentTrack = result;

    ui->artistTrackLabel->setResult( result );
    ui->albumLabel->setResult( result );
    ui->ownerLabel->setText( result->friendlySource() );
    ui->coverImage->setPixmap( m_defaultCover );

    ui->timeLabel->setText( TomahawkUtils::timeToString( 0 ) );
    ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( result.data()->duration() ) );

    ui->stackedLayout->setCurrentWidget( ui->pauseButton );

    ui->loveButton->setEnabled( true );
    ui->loveButton->setVisible( true );

    result->loadSocialActions();

    connect( result.data(), SIGNAL( socialActionsLoaded() ), SLOT( socialActionsLoaded() ) );
}


void
AudioControls::socialActionsLoaded()
{
    Result* r = qobject_cast< Result* >( sender() );
    Q_ASSERT( r );

    if ( m_currentTrack.data() == r )
    {
        if ( m_currentTrack->loved() )
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
    ui->loveButton->setVisible( true );
    m_sliderTimeLine.resume();
}


void
AudioControls::onPlaybackSeeked( qint64 msec )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO << " setting current timer to " << msec;
    m_sliderTimeLine.setPaused( true );
    m_sliderTimeLine.setCurrentTime( msec );
    m_seekMsecs = msec;
}


void
AudioControls::onPlaybackStopped()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    m_currentTrack.clear();

    ui->artistTrackLabel->setText( "" );
    ui->albumLabel->setText( "" );
    ui->ownerLabel->setText( "" );
    ui->timeLabel->setText( "" );
    ui->timeLeftLabel->setText( "" );
    ui->coverImage->setPixmap( QPixmap() );
    ui->seekSlider->setVisible( false );
    m_sliderTimeLine.stop();
    m_sliderTimeLine.setCurrentTime( 0 );

    ui->stackedLayout->setCurrentWidget( ui->playPauseButton );
    ui->loveButton->setEnabled( false );
    ui->loveButton->setVisible( false );
}


void
AudioControls::onPlaybackTimer( qint64 msElapsed )
{
    //tDebug( LOGEXTRA ) << Q_FUNC_INFO << " msElapsed = " << msElapsed << " and timer current time = " << m_sliderTimeLine.currentTime() << " and m_seekMsecs = " << m_seekMsecs;
    if ( m_currentTrack.isNull() )
        return;

    ui->seekSlider->blockSignals( true );

    const int seconds = msElapsed / 1000;
    ui->timeLabel->setText( TomahawkUtils::timeToString( seconds ) );
    ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( m_currentTrack->duration() - seconds ) );

    if ( m_noTimeChange )
    {
        if ( m_sliderTimeLine.currentTime() != msElapsed )
        {
            m_noTimeChange = false;
            m_sliderTimeLine.resume();
        }
    }
    else if ( m_sliderTimeLine.currentTime() >= msElapsed || m_seekMsecs != -1 )
    {
        m_sliderTimeLine.setPaused( true );

        m_noTimeChange = false;
        if ( m_sliderTimeLine.currentTime() == msElapsed )
            m_noTimeChange = true;

        m_sliderTimeLine.setCurrentTime( msElapsed );
        m_seekMsecs = -1;
        if ( AudioEngine::instance()->state() != AudioEngine::Paused )
            m_sliderTimeLine.resume();
    }
    else if ( m_sliderTimeLine.duration() > msElapsed && m_sliderTimeLine.state() == QTimeLine::NotRunning )
    {
        ui->seekSlider->setEnabled( AudioEngine::instance()->canSeek() );
        m_sliderTimeLine.resume();
    }
    else if ( m_sliderTimeLine.state() == QTimeLine::Paused && AudioEngine::instance()->state() != AudioEngine::Paused )
    {
        ui->seekSlider->setEnabled( AudioEngine::instance()->canSeek() );
        m_sliderTimeLine.resume();
    }

    ui->seekSlider->blockSignals( false );
}


void
AudioControls::onRepeatModeChanged( PlaylistInterface::RepeatMode mode )
{
    m_repeatMode = mode;

    switch ( m_repeatMode )
    {
        case PlaylistInterface::NoRepeat:
        {
            // switch to RepeatOne
            ui->repeatButton->setPixmap( RESPATH "images/repeat-off-rest.png" );
            ui->repeatButton->setPixmap( RESPATH "images/repeat-off-pressed.png", QIcon::Off, QIcon::Active );
        }
        break;

        case PlaylistInterface::RepeatOne:
        {
            // switch to RepeatAll
            ui->repeatButton->setPixmap( RESPATH "images/repeat-1-on-rest.png" );
            ui->repeatButton->setPixmap( RESPATH "images/repeat-1-on-pressed.png", QIcon::Off, QIcon::Active );
        }
        break;

        case PlaylistInterface::RepeatAll:
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
        case PlaylistInterface::NoRepeat:
        {
            // switch to RepeatOne
            ViewManager::instance()->setRepeatMode( PlaylistInterface::RepeatOne );
        }
        break;

        case PlaylistInterface::RepeatOne:
        {
            // switch to RepeatAll
            ViewManager::instance()->setRepeatMode( PlaylistInterface::RepeatAll );
        }
        break;

        case PlaylistInterface::RepeatAll:
        {
            // switch to NoRepeat
            ViewManager::instance()->setRepeatMode( PlaylistInterface::NoRepeat );
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
    ViewManager::instance()->showCurrentTrack();
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

        // just queue the rest
        for ( int i = 1; i < tracks.size(); i++ )
        {
            ViewManager::instance()->queue()->model()->append( tracks[ i ] );
        }
    }
}


void
AudioControls::onLoveButtonClicked( bool checked )
{
    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    trackInfo["title"] = m_currentTrack->track();
    trackInfo["artist"] = m_currentTrack->artist()->name();
    trackInfo["album"] = m_currentTrack->album()->name();

    if ( checked )
    {
        Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo(
            s_acInfoIdentifier, Tomahawk::InfoSystem::InfoLove,
            QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo ) );

        DatabaseCommand_SocialAction* cmd = new DatabaseCommand_SocialAction( m_currentTrack, QString( "Love" ), QString( "true") );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        ui->loveButton->setPixmap( RESPATH "images/loved.png" );
    }
    else
    {
        Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo(
            s_acInfoIdentifier, Tomahawk::InfoSystem::InfoUnLove,
            QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo ) );

        DatabaseCommand_SocialAction* cmd = new DatabaseCommand_SocialAction( m_currentTrack, QString( "Love" ), QString( "false" ) );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        ui->loveButton->setPixmap( RESPATH "images/not-loved.png" );
    }
}

