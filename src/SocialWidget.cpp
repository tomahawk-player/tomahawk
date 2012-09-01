/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "SocialWidget.h"
#include "ui_SocialWidget.h"

#include <QPainter>
#include <QDialog>
#include <QPropertyAnimation>

#include "GlobalActionManager.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "Source.h"

#define CORNER_ROUNDNESS 8.0
#define FADING_DURATION 500
#define OPACITY 0.85


SocialWidget::SocialWidget( QWidget* parent )
    : QWidget( parent ) // this is on purpose!
    , ui( new Ui::SocialWidget )
    , m_opacity( 0.00 )
    , m_parent( parent )
    , m_parentRect( parent->rect() )
{
    ui->setupUi( this );

    setAttribute( Qt::WA_TranslucentBackground, true );
    setOpacity( m_opacity );

    m_timer.setSingleShot( true );
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( hide() ) );

    ui->charsLeftLabel->setForegroundRole( QPalette::BrightText );
    ui->buttonBox->button( QDialogButtonBox::Ok )->setText( tr( "Tweet" ) );
    
    m_parent->installEventFilter( this );

    connect( ui->buttonBox, SIGNAL( accepted() ), SLOT( accept() ) );
    connect( ui->buttonBox, SIGNAL( rejected() ), SLOT( close() ) );
    connect( ui->textEdit, SIGNAL( textChanged() ), SLOT( onChanged() ) );
    connect( ui->facebookButton, SIGNAL( clicked( bool ) ), SLOT( onChanged() ) );
    connect( ui->twitterButton, SIGNAL( clicked( bool ) ), SLOT( onChanged() ) );
    connect( GlobalActionManager::instance(), SIGNAL( shortLinkReady( QUrl, QUrl, QVariant ) ), SLOT( onShortLinkReady( QUrl, QUrl, QVariant ) ) );

    onChanged();

    ui->twitterButton->setChecked( true );
    ui->twitterButton->setVisible( false );
    ui->facebookButton->setVisible( false );
}


SocialWidget::~SocialWidget()
{
    delete ui;
}


void
SocialWidget::setOpacity( qreal opacity )
{
    m_opacity = opacity;

    if ( m_opacity == 0.00 && !isHidden() )
    {
        QWidget::hide();
        emit hidden();
    }
    else if ( m_opacity > 0.00 && isHidden() )
    {
        QWidget::show();
    }

    repaint();
}


void
SocialWidget::setPosition( QPoint position )
{
    m_position = position;
    onGeometryUpdate();
}


void
SocialWidget::show( int timeoutSecs )
{
    if ( !isEnabled() )
        return;

    QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
    animation->setDuration( FADING_DURATION );
    animation->setEndValue( 1.0 );
    animation->start();

    if( timeoutSecs > 0 )
        m_timer.start( timeoutSecs * 1000 );
}


void
SocialWidget::hide()
{
    if ( !isEnabled() )
        return;

    QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
    animation->setDuration( FADING_DURATION );
    animation->setEndValue( 0.00 );
    animation->start();
}


bool
SocialWidget::shown() const
{
    if ( !isEnabled() )
        return false;

    return m_opacity == OPACITY;
}


void
SocialWidget::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );

    QPainter p( this );
    QRect r = contentsRect();

    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );
    p.setOpacity( m_opacity );

    QPen pen( palette().dark().color(), .5 );
    p.setPen( pen );
    p.setBrush( QColor( 30, 30, 30, 255.0 * OPACITY ) );

    p.drawRoundedRect( r, CORNER_ROUNDNESS, CORNER_ROUNDNESS );

    QWidget::paintEvent( event );
    return;

    QTextOption to( Qt::AlignCenter );
    to.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );

    QFont f( font() );
    f.setPointSize( TomahawkUtils::defaultFontSize() + 7 );
    f.setBold( true );

    QRectF textRect = r.adjusted( 8, 8, -8, -8 );
    qreal availHeight = textRect.height();

    QFontMetricsF fm( f );
    qreal textHeight = fm.boundingRect( textRect, Qt::AlignCenter | Qt::TextWordWrap, "SocialWidget" ).height();
    while( textHeight > availHeight )
    {
        if( f.pointSize() <= 4 ) // don't try harder
            break;

        f.setPointSize( f.pointSize() - 1 );
        fm = QFontMetricsF( f );
        textHeight = fm.boundingRect( textRect, Qt::AlignCenter | Qt::TextWordWrap, "SocialWidget" ).height();
    }

    p.setFont( f );
    p.setPen( palette().highlightedText().color() );
    p.drawText( r.adjusted( 8, 8, -8, -8 ), "SocialWidget", to );
}


void
SocialWidget::onShortLinkReady( const QUrl& longUrl, const QUrl& shortUrl, const QVariant& callbackObj )
{
    Q_UNUSED( longUrl );
    Q_UNUSED( callbackObj );

    if ( m_query->album().isEmpty() )
        ui->textEdit->setText( tr( "Listening to \"%1\" by %2. %3" ).arg( m_query->track() ).arg( m_query->artist() ).arg( shortUrl.toString() ) );
    else
        ui->textEdit->setText( tr( "Listening to \"%1\" by %2 on \"%3\". %4" ).arg( m_query->track() ).arg( m_query->artist() ).arg( m_query->album() ).arg( shortUrl.toString() ) );
}


void
SocialWidget::setQuery( const Tomahawk::query_ptr& query )
{
    m_query = query;
    ui->coverImage->setPixmap( query->cover( ui->coverImage->size() ) );
    onShortLinkReady( QString(), QString(), QVariant() );
    onChanged();

    QUrl longUrl = GlobalActionManager::instance()->openLinkFromQuery( query );
    GlobalActionManager::instance()->shortenLink( longUrl );
}


void
SocialWidget::onChanged()
{
    const int remaining = charsAvailable() - ui->textEdit->toPlainText().length();
    ui->charsLeftLabel->setText( tr( "%1 characters left" ).arg( remaining ) );
    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( remaining >= 0 && ( ui->facebookButton->isChecked() || ui->twitterButton->isChecked() ) );
}


void
SocialWidget::accept()
{
    tDebug() << "Sharing social link!";
    
    QVariantMap shareInfo;
    Tomahawk::InfoSystem::InfoStringHash trackInfo;

    trackInfo["title"] = m_query->track();
    trackInfo["artist"] = m_query->artist();
    trackInfo["album"] = m_query->album();

    shareInfo["trackinfo"] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    shareInfo["message"] = ui->textEdit->toPlainText();
    shareInfo["accountlist"] = QStringList( "all" );

    Tomahawk::InfoSystem::InfoPushData pushData( uuid(), Tomahawk::InfoSystem::InfoShareTrack, shareInfo, Tomahawk::InfoSystem::PushNoFlag );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );

    deleteLater();
}


void
SocialWidget::close()
{
    QWidget::hide();
    deleteLater();
}


unsigned int
SocialWidget::charsAvailable() const
{
    if ( ui->twitterButton->isChecked() )
        return 140;

    return 420; // facebook max length
}


void
SocialWidget::onGeometryUpdate()
{
    QPoint p( m_parent->rect().width() - m_parentRect.width(), m_parent->rect().height() - m_parentRect.height() );
    m_position += p;
    m_parentRect = m_parent->rect();

    QPoint position( m_position - QPoint( size().width(), size().height() ) );
    if ( position != pos() )
    {
        move( position );
    }
}


bool
SocialWidget::eventFilter( QObject* object, QEvent* event )
{
    if ( event->type() == QEvent::Resize )
    {
        onGeometryUpdate();
    }

    return QObject::eventFilter( object, event );
}
