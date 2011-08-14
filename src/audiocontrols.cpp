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

#include <QNetworkReply>
#include <QDropEvent>
#include <QMouseEvent>

#include "audio/audioengine.h"
#include "viewmanager.h"
#include "playlist/playlistview.h"
#include "database/database.h"
#include "database/databasecommand_socialaction.h"

#include "album.h"

#include "utils/imagebutton.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"
#include <globalactionmanager.h>

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

    ui->ownerLabel->setForegroundRole( QPalette::Dark );
    ui->metaDataArea->setStyleSheet( "QWidget#metaDataArea {\nborder-width: 4px;\nborder-image: url(" RESPATH "images/now-playing-panel.png) 4 4 4 4 stretch stretch; }" );

    ui->seekSlider->setFixedHeight( 20 );
    ui->seekSlider->setEnabled( true );
    ui->seekSlider->setStyleSheet( "QSlider::groove::horizontal {"
                                   "margin: 5px; border-width: 3px;"
                                   "border-image: url(" RESPATH "images/seek-slider-bkg.png) 3 3 3 3 stretch stretch;"
                                   "}"

                                   "QSlider::sub-page:horizontal {"
                                   "margin: 5px; border-width: 3px;"
                                   "border-image: url(" RESPATH "images/seek-slider-level.png) 3 3 3 3 stretch stretch;"
                                   "}"

                                   "QSlider::handle::horizontal {"
                                   "margin-bottom: -7px; margin-top: -7px;"
                                   "margin-left: -4px; margin-right: -4px;"
                                   "height: 17px; width: 16px;"
                                   "background-image: url(" RESPATH "images/seek-and-volume-knob-rest.png);"
                                   "background-repeat: no-repeat;"
                                   "}" );

    ui->volumeSlider->setFixedHeight( 20 );
    ui->volumeSlider->setRange( 0, 100 );
    ui->volumeSlider->setValue( AudioEngine::instance()->volume() );
    ui->volumeSlider->setStyleSheet( "QSlider::groove::horizontal {"
                                     "margin: 5px; border-width: 3px;"
                                     "border-image: url(" RESPATH "images/volume-slider-bkg.png) 3 3 3 3 stretch stretch;"
                                     "}"

                                     "QSlider::sub-page:horizontal {"
                                     "margin: 5px; border-width: 3px;"
                                     "border-image: url(" RESPATH "images/seek-slider-level.png) 3 3 3 3 stretch stretch;"
                                     "}"

                                     "QSlider::handle::horizontal {"
                                     "margin-bottom: -7px; margin-top: -7px;"
                                     "margin-left: -4px; margin-right: -4px;"
                                     "height: 17px; width: 16px;"
                                     "background-image: url(" RESPATH "images/seek-and-volume-knob-rest.png);"
                                     "background-repeat: no-repeat;"
                                     "}" );

/*    m_playAction  = new QAction( this );
    m_pauseAction = new QAction( this );
    m_prevAction  = new QAction( this );
    m_nextAction  = new QAction( this );

    connect( m_playAction,  SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( play() ) );
    connect( m_pauseAction, SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( pause() ) );
    connect( m_prevAction,  SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( previous() ) );
    connect( m_nextAction,  SIGNAL( triggered() ), (QObject*)APP->audioEngine(), SLOT( next() ) ); */

    connect( ui->seekSlider,       SIGNAL( valueChanged( int ) ), AudioEngine::instance(), SLOT( seek( int ) ) );
    connect( ui->volumeSlider,     SIGNAL( valueChanged( int ) ), AudioEngine::instance(), SLOT( setVolume( int ) ) );
    connect( ui->prevButton,       SIGNAL( clicked() ), AudioEngine::instance(), SLOT( previous() ) );
    connect( ui->playPauseButton,  SIGNAL( clicked() ), AudioEngine::instance(), SLOT( play() ) );
    connect( ui->pauseButton,      SIGNAL( clicked() ), AudioEngine::instance(), SLOT( pause() ) );
    connect( ui->nextButton,       SIGNAL( clicked() ), AudioEngine::instance(), SLOT( next() ) );
    connect( ui->volumeLowButton,  SIGNAL( clicked() ), AudioEngine::instance(), SLOT( lowerVolume() ) );
    connect( ui->volumeHighButton, SIGNAL( clicked() ), AudioEngine::instance(), SLOT( raiseVolume() ) );

    connect( ui->playPauseButton,  SIGNAL( clicked() ), this, SIGNAL( playPressed() ) );
    connect( ui->pauseButton,  SIGNAL( clicked() ), this,     SIGNAL( pausePressed() ) );

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
    connect( AudioEngine::instance(), SIGNAL( timerMilliSeconds( qint64 ) ), SLOT( onPlaybackTimer( qint64 ) ) );
    connect( AudioEngine::instance(), SIGNAL( volumeChanged( int ) ), SLOT( onVolumeChanged( int ) ) );

    m_defaultCover = QPixmap( RESPATH "images/no-album-art-placeholder.png" )
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

    onPlaybackLoading( result );

    Tomahawk::InfoSystem::InfoCriteriaHash trackInfo;
    trackInfo["artist"] = result->artist()->name();
    trackInfo["album"] = result->album()->name();

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_acInfoIdentifier;
    requestData.type = Tomahawk::InfoSystem::InfoAlbumCoverArt;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo );
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
    ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( result->duration() ) );

    ui->seekSlider->setRange( 0, m_currentTrack->duration() * 1000 );
    ui->seekSlider->setValue( 0 );
    ui->seekSlider->setVisible( true );

/*    m_playAction->setEnabled( false );
    m_pauseAction->setEnabled( true ); */

    ui->stackedLayout->setCurrentWidget( ui->pauseButton );

    ui->loveButton->setEnabled( true );
    ui->loveButton->setVisible( true );

    result->loadSocialActions();

    connect( result.data(), SIGNAL( socialActionsLoaded() ), this, SLOT( socialActionsLoaded() ) );
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
/*    m_pauseAction->setEnabled( false );
    m_playAction->setEnabled( true ); */

    ui->stackedLayout->setCurrentWidget( ui->playPauseButton );
}

void
AudioControls::onPlaybackResumed()
{
/*    m_playAction->setEnabled( false );
    m_pauseAction->setEnabled( true ); */

    ui->stackedLayout->setCurrentWidget( ui->pauseButton );
    ui->loveButton->setVisible( true );
}


void
AudioControls::onPlaybackStopped()
{
    m_currentTrack.clear();

    ui->artistTrackLabel->setText( "" );
    ui->albumLabel->setText( "" );
    ui->ownerLabel->setText( "" );
    ui->timeLabel->setText( "" );
    ui->timeLeftLabel->setText( "" );
    ui->coverImage->setPixmap( QPixmap() );
    ui->seekSlider->setVisible( false );

    ui->stackedLayout->setCurrentWidget( ui->playPauseButton );
    ui->loveButton->setEnabled( false );
    ui->loveButton->setVisible( false );

/*    m_pauseAction->setEnabled( false );
    m_playAction->setEnabled( true ); */
}


void
AudioControls::onPlaybackTimer( qint64 msElapsed )
{
    if ( m_currentTrack.isNull() )
        return;

    ui->seekSlider->blockSignals( true );

    const int seconds = msElapsed / 1000;
    ui->timeLabel->setText( TomahawkUtils::timeToString( seconds ) );
    ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( m_currentTrack->duration() - seconds ) );
    ui->seekSlider->setValue( msElapsed );

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
    if ( GlobalActionManager::instance()->acceptsMimeData( e->mimeData() ) )
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
    if ( GlobalActionManager::instance()->acceptsMimeData( e->mimeData() ) )
    {
        connect( GlobalActionManager::instance(), SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( droppedTracks( QList<Tomahawk::query_ptr> ) ) );
        GlobalActionManager::instance()->tracksFromMimeData( e->mimeData() );

        e->accept();
    }
}


void
AudioControls::droppedTracks( QList< query_ptr > tracks )
{
    disconnect( GlobalActionManager::instance(), SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( droppedTracks( QList<Tomahawk::query_ptr> ) ) );

    if ( !tracks.isEmpty() )
    {
        // queue and play the first if nothign is playing
        GlobalActionManager::instance()->handleOpenTrack( tracks.first() );

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
    Tomahawk::InfoSystem::InfoCriteriaHash trackInfo;
    trackInfo["title"] = m_currentTrack->track();
    trackInfo["artist"] = m_currentTrack->artist()->name();
    trackInfo["album"] = m_currentTrack->album()->name();

    if ( checked )
    {
        Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo(
            s_acInfoIdentifier, Tomahawk::InfoSystem::InfoLove,
            QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo ) );

        DatabaseCommand_SocialAction* cmd = new DatabaseCommand_SocialAction( m_currentTrack, QString( "Love" ), QString( "true") );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        ui->loveButton->setPixmap( RESPATH "images/loved.png" );
    }
    else
    {
        Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo(
            s_acInfoIdentifier, Tomahawk::InfoSystem::InfoUnLove,
            QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo ) );

        DatabaseCommand_SocialAction* cmd = new DatabaseCommand_SocialAction( m_currentTrack, QString( "Love" ), QString( "false" ) );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        ui->loveButton->setPixmap( RESPATH "images/not-loved.png" );
    }
}

