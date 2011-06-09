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

#include "audio/audioengine.h"
#include "viewmanager.h"
#include "utils/imagebutton.h"
#include "utils/tomahawkutils.h"

#include "album.h"

static QString s_acInfoIdentifier = QString( "AUDIOCONTROLS" );


AudioControls::AudioControls( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::AudioControls )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
    ui->setupUi( this );

    ui->buttonAreaLayout->setSpacing( 2 );

    QFont font( ui->artistTrackLabel->font() );
    font.setPixelSize( 12 );

#ifdef Q_WS_MAC
    font.setPointSize( font.pointSize() - 2 );
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
        SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
        SLOT( infoSystemInfo( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

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
    qDebug() << Q_FUNC_INFO;

    onPlaybackLoading( result );

    Tomahawk::InfoSystem::InfoCriteriaHash trackInfo;
    trackInfo["artist"] = result->artist()->name();
    trackInfo["album"] = result->album()->name();

    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo(
        s_acInfoIdentifier, Tomahawk::InfoSystem::InfoAlbumCoverArt,
        QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo ), Tomahawk::InfoSystem::InfoCustomData() );
}


void
AudioControls::infoSystemInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomData customData )
{
    Q_UNUSED( input );
    Q_UNUSED( customData );

    qDebug() << Q_FUNC_INFO << caller << type << s_acInfoIdentifier << Tomahawk::InfoSystem::InfoAlbumCoverArt;
    if ( caller != s_acInfoIdentifier || type != Tomahawk::InfoSystem::InfoAlbumCoverArt )
    {
        qDebug() << "Info of wrong type or not with our identifier";
        return;
    }

    if ( m_currentTrack.isNull() )
    {
        qDebug() << "Current track is null when trying to apply fetched cover art";
        return;
    }

    if ( !output.canConvert< Tomahawk::InfoSystem::InfoCustomData >() )
    {
        qDebug() << "Cannot convert fetched art from a QByteArray";
        return;
    }

    Tomahawk::InfoSystem::InfoCustomData returnedData = output.value< Tomahawk::InfoSystem::InfoCustomData >();
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
    qDebug() << Q_FUNC_INFO;
}


void
AudioControls::onPlaybackLoading( const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO;

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

    ui->pauseButton->setEnabled( true );
    ui->pauseButton->setVisible( true );
    ui->playPauseButton->setVisible( false );
    ui->playPauseButton->setEnabled( false );
}


void
AudioControls::onPlaybackPaused()
{
/*    m_pauseAction->setEnabled( false );
    m_playAction->setEnabled( true ); */

    ui->pauseButton->setVisible( false );
    ui->pauseButton->setEnabled( false );
    ui->playPauseButton->setEnabled( true );
    ui->playPauseButton->setVisible( true );
}


void
AudioControls::onPlaybackResumed()
{
/*    m_playAction->setEnabled( false );
    m_pauseAction->setEnabled( true ); */

    ui->pauseButton->setVisible( true );
    ui->pauseButton->setEnabled( true );
    ui->playPauseButton->setVisible( false );
    ui->playPauseButton->setEnabled( false );
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

    ui->pauseButton->setVisible( false );
    ui->pauseButton->setEnabled( false );
    ui->playPauseButton->setEnabled( true );
    ui->playPauseButton->setVisible( true );

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
