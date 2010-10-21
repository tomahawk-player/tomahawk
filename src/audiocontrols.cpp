#include "audiocontrols.h"
#include "ui_audiocontrols.h"

#include <QNetworkReply>

#include "tomahawk/tomahawkapp.h"
#include "utils/tomahawkutils.h"

#include "audioengine.h"
#include "imagebutton.h"
#include "playlist/playlistmanager.h"

#define LASTFM_DEFAULT_COVER "http://cdn.last.fm/flatness/catalogue/noimage"


AudioControls::AudioControls( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::AudioControls )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
    ui->setupUi( this );

    ui->buttonAreaLayout->setSpacing( 2 );
    ui->trackLabelLayout->setSpacing( 3 );

    QFont font( ui->artistTrackLabel->font() );
    font.setPixelSize( 12 );

    ui->artistTrackLabel->setFont( font );
    ui->albumLabel->setFont( font );
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
    ui->metadataArea->setStyleSheet( "QWidget#metadataArea {\nborder-width: 4px;\nborder-image: url(" RESPATH "images/now-playing-panel.png) 4 4 4 4 stretch stretch; }" );

    ui->seekSlider->setFixedHeight( 20 );
    ui->seekSlider->setEnabled( false );
    ui->seekSlider->setStyleSheet( "QSlider::groove::horizontal {"
                                   "margin: 5px; border-width: 3px;"
                                   "border-image: url(" RESPATH "images/seek-slider-bkg.png) 3 3 3 3 stretch stretch;"
                                   "}"

                                   "QSlider::handle::horizontal {"
                                   "margin-left: 5px; margin-right: -5px; "
                                   "width: 0px;"

                                   //"margin-bottom: -7px; margin-top: -7px;"
                                   //"height: 17px; width: 16px;"
                                   //"background-image: url(" RESPATH "images/seek-and-volume-knob-rest.png);"
                                   //"background-repeat: no-repeat;"
                                   "}"

                                   "QSlider::sub-page:horizontal {"
                                   "margin: 5px; border-width: 3px;"
                                   "border-image: url(" RESPATH "images/seek-slider-level.png) 3 3 3 3 stretch stretch;"
                                   "}"
                                  );

    ui->volumeSlider->setFixedHeight( 20 );
    ui->volumeSlider->setRange( 0, 100 );
    ui->volumeSlider->setValue( APP->audioEngine()->volume() );
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
                                     "}"

                                     );

/*    m_playAction  = new QAction( this );
    m_pauseAction = new QAction( this );
    m_prevAction  = new QAction( this );
    m_nextAction  = new QAction( this );

    connect( m_playAction,  SIGNAL( triggered() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( play() ) );
    connect( m_pauseAction, SIGNAL( triggered() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( pause() ) );
    connect( m_prevAction,  SIGNAL( triggered() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( previous() ) );
    connect( m_nextAction,  SIGNAL( triggered() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( next() ) ); */

    connect( ui->volumeSlider,     SIGNAL( valueChanged( int ) ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( setVolume( int ) ) );
    connect( ui->prevButton,       SIGNAL( clicked() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( previous() ) );
    connect( ui->playPauseButton,  SIGNAL( clicked() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( play() ) );
    connect( ui->pauseButton,      SIGNAL( clicked() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( pause() ) );
    connect( ui->nextButton,       SIGNAL( clicked() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( next() ) );
    connect( ui->volumeLowButton,  SIGNAL( clicked() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( lowerVolume() ) );
    connect( ui->volumeHighButton, SIGNAL( clicked() ), (QObject*)TomahawkApp::instance()->audioEngine(), SLOT( raiseVolume() ) );

    connect( ui->repeatButton,     SIGNAL( clicked() ), SLOT( onRepeatClicked() ) );
    connect( ui->shuffleButton,    SIGNAL( clicked() ), SLOT( onShuffleClicked() ) );

    // <From AudioEngine>
    connect( (QObject*)TomahawkApp::instance()->audioEngine(), SIGNAL( loading( const Tomahawk::result_ptr& ) ), SLOT( onPlaybackLoading( const Tomahawk::result_ptr& ) ) );
    connect( (QObject*)TomahawkApp::instance()->audioEngine(), SIGNAL( started( const Tomahawk::result_ptr& ) ), SLOT( onPlaybackStarted( const Tomahawk::result_ptr& ) ) );
    connect( (QObject*)TomahawkApp::instance()->audioEngine(), SIGNAL( paused() ), SLOT( onPlaybackPaused() ) );
    connect( (QObject*)TomahawkApp::instance()->audioEngine(), SIGNAL( resumed() ), SLOT( onPlaybackResumed() ) );
    connect( (QObject*)TomahawkApp::instance()->audioEngine(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ) );
    connect( (QObject*)TomahawkApp::instance()->audioEngine(), SIGNAL( timerSeconds( unsigned int ) ), SLOT( onPlaybackTimer( unsigned int ) ) );
    connect( (QObject*)TomahawkApp::instance()->audioEngine(), SIGNAL( volumeChanged( int ) ), SLOT( onVolumeChanged( int ) ) );

    m_defaultCover = QPixmap( RESPATH "images/no-album-art-placeholder.png" )
                     .scaled( ui->coverImage->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

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
AudioControls::onCoverArtDownloaded()
{
    if ( m_currentTrack.isNull() )
        return;

    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isEmpty() )
    {
        const QByteArray ba = reply->readAll();
        if ( ba.length() )
        {
            QPixmap pm;
            pm.loadFromData( ba );

            if ( pm.isNull() || reply->url().toString().startsWith( LASTFM_DEFAULT_COVER ) )
                ui->coverImage->setPixmap( m_defaultCover );
            else
                ui->coverImage->setPixmap( pm.scaled( ui->coverImage->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
        }
    }
    else
    {
//        qDebug() << "Following redirect to" << redir.toString();
        QNetworkRequest req( redir );
        QNetworkReply* reply = APP->nam()->get( req );
        connect( reply, SIGNAL( finished() ), SLOT( onCoverArtDownloaded() ) );
    }

    reply->deleteLater();
}


void
AudioControls::onPlaybackStarted( const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO;

    onPlaybackLoading( result );

    QString imgurl = "http://ws.audioscrobbler.com/2.0/?method=album.imageredirect&artist=%1&album=%2&size=medium&api_key=7a90f6672a04b809ee309af169f34b8b";
    QNetworkRequest req( imgurl.arg( result->artist() ).arg( result->album() ) );
    QNetworkReply* reply = APP->nam()->get( req );
    connect( reply, SIGNAL( finished() ), SLOT( onCoverArtDownloaded() ) );
}


void
AudioControls::onPlaybackLoading( const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO;

    m_currentTrack = result;

    ui->artistTrackLabel->setText( QString( "%1 - %2" ).arg( result->artist() ).arg( result->track() ) );
    ui->albumLabel->setText( result->album() );
    ui->ownerLabel->setText( result->collection()->source()->friendlyName() );
    ui->coverImage->setPixmap( m_defaultCover );

    if ( ui->timeLabel->text().isEmpty() )
        ui->timeLabel->setText( "00:00" );

    if ( ui->timeLeftLabel->text().isEmpty() )
        ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( result->duration() ) );

    ui->seekSlider->setRange( 0, m_currentTrack->duration() );
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
AudioControls::onPlaybackTimer( unsigned int seconds )
{
    if ( m_currentTrack.isNull() )
        return;

    ui->timeLabel->setText( TomahawkUtils::timeToString( seconds ) );
    ui->timeLeftLabel->setText( "-" + TomahawkUtils::timeToString( m_currentTrack->duration() - seconds ) );
    ui->seekSlider->setValue( seconds );
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
            APP->playlistManager()->setRepeatMode( PlaylistInterface::RepeatOne );
        }
        break;

        case PlaylistInterface::RepeatOne:
        {
            // switch to RepeatAll
            APP->playlistManager()->setRepeatMode( PlaylistInterface::RepeatAll );
        }
        break;

        case PlaylistInterface::RepeatAll:
        {
            // switch to NoRepeat
            APP->playlistManager()->setRepeatMode( PlaylistInterface::NoRepeat );
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
    APP->playlistManager()->setShuffled( m_shuffled ^ true );
}
