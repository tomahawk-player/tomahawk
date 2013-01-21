/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012, Teo Mrnjavac <teo@kde.org>
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

#include "GlobalActionManager.h"
#include "Source.h"

#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QPainter>
#include <QDialog>
#include <QPropertyAnimation>

#define ARROW_HEIGHT 6


SocialWidget::SocialWidget( QWidget* parent )
    : QWidget( parent ) // this is on purpose!
    , ui( new Ui::SocialWidget )
    , m_parent( parent )
    , m_parentRect( parent->rect() )
{
    ui->setupUi( this );
    setWindowFlags( Qt::Popup | Qt::FramelessWindowHint );

    setAutoFillBackground( false );
    setAttribute( Qt::WA_TranslucentBackground, true );
    setAttribute( Qt::WA_NoSystemBackground, true );

    TomahawkUtils::unmarginLayout( layout() );

#ifndef Q_OS_MAC
    ui->verticalLayout->setContentsMargins( 12, 4, 12, 12 );
    setContentsMargins( contentsMargins().left() + 2, contentsMargins().top() + 2,
                        contentsMargins().right() + 2, contentsMargins().bottom() + 2 + ARROW_HEIGHT );
#else
    ui->verticalLayout->setContentsMargins( 12, 0, 12, 16 );
    setContentsMargins( contentsMargins().left() + 2, 4,
                        contentsMargins().right() + 2, contentsMargins().bottom() + 2 + ARROW_HEIGHT );
    ui->horizontalLayout->setContentsMargins( 0, 0, 0, 0 );
#endif


    m_timer.setSingleShot( true );
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( hide() ) );

    ui->charsLeftLabel->setForegroundRole( QPalette::Text );
    ui->charsLeftLabel->setStyleSheet( "text: black" );
    ui->buttonBox->button( QDialogButtonBox::Ok )->setText( tr( "Tweet" ) );
    ui->buttonBox->button( QDialogButtonBox::Ok )->setIcon( ImageRegistry::instance()->icon( RESPATH "images/tweet.svg" ) );
    ui->buttonBox->button( QDialogButtonBox::Cancel )->setIcon( ImageRegistry::instance()->icon( RESPATH "images/cancel.svg" ) );

    ui->textEdit->setStyleSheet( "border: 1px solid " + TomahawkUtils::Colors::BORDER_LINE.name() );

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

    if( timeoutSecs > 0 )
        m_timer.start( timeoutSecs * 1000 );

    QWidget::show();
}


void
SocialWidget::hide()
{
    if ( !isEnabled() )
        return;

    QWidget::hide();
}


bool
SocialWidget::shown() const
{
    if ( !isEnabled() )
        return false;

    return isVisible();
}


void
SocialWidget::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );

    QPainterPath outline;

    QRect r = contentsRect();
    outline.addRoundedRect( r, TomahawkUtils::POPUP_ROUNDING_RADIUS, TomahawkUtils::POPUP_ROUNDING_RADIUS );
    outline.moveTo( r.right() - ARROW_HEIGHT * 2, r.bottom()+1 );
    outline.lineTo( r.right() - ARROW_HEIGHT * 3, r.bottom()+1 + ARROW_HEIGHT );
    outline.lineTo( r.right() - ARROW_HEIGHT * 4, r.bottom()+1 );

    TomahawkUtils::drawCompositedPopup( this,
                                        outline,
                                        TomahawkUtils::Colors::BORDER_LINE,
                                        TomahawkUtils::Colors::POPUP_BACKGROUND,
                                        TomahawkUtils::POPUP_OPACITY );
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
    ui->coverImage->setPixmap( TomahawkUtils::addDropShadow( query->cover( ui->coverImage->size() ), ui->coverImage->size() ) );
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

    QPoint position( m_position - QPoint( size().width(), size().height() )
                     + QPoint( 2 + ARROW_HEIGHT * 3, 0 ) );

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


Tomahawk::query_ptr
SocialWidget::query() const
{
    return m_query;
}


QPoint
SocialWidget::position() const
{
    return m_position;
}
