#include "topbar.h"
#include "ui_topbar.h"

#include <QLabel>
#include <QPropertyAnimation>
#include <QRadioButton>
#include <QFile>

#include "tomahawk/functimeout.h"
#include "tomahawk/tomahawkapp.h"

#define MAXDUDES 3
#define DUDEWIDTH 10
#define DUDEX( i ) ( DUDEWIDTH * i )

using namespace Tomahawk;


TopBar::TopBar( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::TopBar )
    , m_sources( 0 )
    , m_tracks( 0 )
    , m_artists( 0 )
    , m_shown( 0 )
{
    ui->setupUi( this );

    ui->statsLabelNumTracks->setFormat( "%L1 " + tr( "Tracks" ) );
    ui->statsLabelNumArtists->setFormat( "%L1 " + tr( "Artists" ) );
    connect( ui->filterEdit, SIGNAL( textChanged( QString ) ), SIGNAL( filterTextChanged( QString ) ) );

    ui->filterEdit->setStyleSheet( "QLineEdit { border: 1px solid gray; border-radius: 6px; margin-right: 2px; }" );
#ifdef Q_WS_MAC
    ui->filterEdit->setAttribute( Qt::WA_MacShowFocusRect, 0 );
#endif

    // initialise dudes
    for( int i = 0; i < MAXDUDES; ++i )
    {
        QLabel* manlbl = new QLabel( ui->widgetMen );
        manlbl->setPixmap( QPixmap( RESPATH "images/avatar-dude.png" ) );
        manlbl->move( QPoint( -10,0 ) );
        manlbl->show();
        m_dudes.append( manlbl );
    }

    QFile f( RESPATH "topbar-radiobuttons.css" );
    f.open( QFile::ReadOnly );
    QString css = QString::fromAscii( f.readAll() );
    f.close();

    ui->widgetRadio->setStyleSheet( css );

    ui->radioNormal->setFocusPolicy( Qt::NoFocus );
    ui->radioDetailed->setFocusPolicy( Qt::NoFocus );
    ui->radioCloud->setFocusPolicy( Qt::NoFocus );

    ui->radioDetailed->setEnabled( false );

    connect( ui->radioNormal, SIGNAL( clicked() ), SIGNAL( flatMode() ) );
    connect( ui->radioDetailed, SIGNAL( clicked() ), SIGNAL( artistMode() ) );
    connect( ui->radioCloud, SIGNAL( clicked() ), SIGNAL( albumMode() ) );

//    ui->widgetRadio->hide(); // FIXME

    setNumSources( 0 );
    setNumTracks( 0 );
    setNumArtists( 0 );
    setNumShown( 0 );

    ui->radioNormal->setChecked( true );
}


TopBar::~TopBar()
{
    delete ui;
}


void
TopBar::changeEvent( QEvent *e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
TopBar::fadeInDude( unsigned int i )
{
//    qDebug() << Q_FUNC_INFO << i;

    QLabel* dude = m_dudes.at( i );
    QPropertyAnimation* ani = new QPropertyAnimation( dude, "pos" );
    ani->setDuration( 1000 );
    ani->setEasingCurve( QEasingCurve::InQuad );
    ani->setStartValue( QPoint( -10,0 ) );
    ani->setEndValue( QPoint( DUDEX( i+1 ), 0 ) );

    qDebug() << "Animating from" << ani->startValue() << "to" << ani->endValue();
    connect( ani, SIGNAL( finished() ), ani, SLOT( deleteLater() ) );
    ani->start();
}


void
TopBar::fadeOutDude( unsigned int i )
{
//    qDebug() << Q_FUNC_INFO << i;

    QLabel* dude = m_dudes.at( i );
    QPropertyAnimation* ani = new QPropertyAnimation( dude, "pos" );
    ani->setDuration( 1000 );
    ani->setEasingCurve( QEasingCurve::OutQuad );
    ani->setStartValue( dude->pos() );
    ani->setEndValue( QPoint( -10, 0 ) );

    qDebug() << "Animating from" << ani->startValue() << "to" << ani->endValue();
    connect( ani, SIGNAL( finished() ), ani, SLOT( deleteLater() ) );
    ani->start();
}


void
TopBar::setNumSources( unsigned int i )
{
//    qDebug() << Q_FUNC_INFO << i;

    // Dude0 Dude1 Dude2
    ui->statsLabelNumSources->setText( QString( "%L1 %2" ).arg( i ).arg( tr( "Sources" ) ) );

    if( ( m_sources >= MAXDUDES && i >= MAXDUDES ) || m_sources == i )
    {
        m_sources = i;
        return;
    }

    if( i > m_sources )
    {
        for( unsigned int k = m_sources; k < MAXDUDES && k < i; ++k )
        {
            fadeInDude( k );
        }
        m_sources = i;
    }
    else
    {
        int k = qMin( (unsigned int)MAXDUDES - 1, m_sources - 1 );
        do
        {
            fadeOutDude( k );
            m_sources--;
        } while( (unsigned int)k-- > i );

        m_sources = i;
    }
}


void
TopBar::setNumTracks( unsigned int i )
{
    m_tracks = i;
    ui->statsLabelNumTracks->setVisible( m_tracks > 0 );
    ui->statsLabelNumTracks->setVal( i );
}


void
TopBar::setNumArtists( unsigned int i )
{
    m_artists = i;
    ui->statsLabelNumArtists->setVisible( m_artists > 0 );
    ui->statsLabelNumArtists->setVal( i );
}


void
TopBar::setNumShown( unsigned int i )
{
    m_shown = i;
    ui->statsLabelNumShown->setVisible( m_shown != m_tracks );
    ui->statsLabelNumShown->setText( QString( "%L1 %2" ).arg( i ).arg( tr( "Shown" ) ) );
}


void
TopBar::addSource()
{
//    qDebug() << Q_FUNC_INFO;
    setNumSources( m_sources + 1 );
}


void
TopBar::removeSource()
{
//    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( m_sources > 0 );
    setNumSources( m_sources - 1 );
}
