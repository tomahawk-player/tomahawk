/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
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

#include "SourceTreePopupDialog.h"

#include <QApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>

#ifdef QT_MAC_USE_COCOA
    #include "SourceTreePopupDialog_mac.h"
#endif

#include "utils/TomahawkUtilsGui.h"
#include "utils/ImageRegistry.h"

using namespace Tomahawk;

SourceTreePopupDialog::SourceTreePopupDialog()
    : QWidget( 0 )
    , m_layout( 0 )
    , m_result( false )
    , m_label( 0 )
    , m_buttons( 0 )
{
#ifndef ENABLE_HEADLESS
    setParent( QApplication::activeWindow() );
#endif
    setWindowFlags( Qt::Popup | Qt::FramelessWindowHint );

    setAutoFillBackground( false );
    setAttribute( Qt::WA_TranslucentBackground, true );
    setAttribute( Qt::WA_NoSystemBackground, true );

    //setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    m_title = new QLabel( this );
    QFont titleFont = m_title->font();
    titleFont.setBold( true );
    m_title->setStyleSheet( "color: " + TomahawkUtils::Colors::GROUP_HEADER.name() );
    titleFont.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
    m_title->setFont( titleFont );
    m_title->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    m_label = new QLabel( this );
    m_buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this );

    m_buttons->button( QDialogButtonBox::Ok )->setIcon( ImageRegistry::instance()->icon( RESPATH "images/delete.svg" ) );
    m_buttons->button( QDialogButtonBox::Cancel )->setIcon( ImageRegistry::instance()->icon( RESPATH "images/cancel.svg" ) );

    connect( m_buttons, SIGNAL( accepted() ), this, SLOT( onAccepted() ) );
    connect( m_buttons, SIGNAL( rejected() ), this, SLOT( onRejected() ) );

    m_layout = new QVBoxLayout;
    TomahawkUtils::unmarginLayout( m_layout );
    setLayout( m_layout );
    m_layout->setSpacing( 8 );
    m_layout->setMargin( 6 );

    m_layout->addWidget( m_title );

    m_separatorLine = new QWidget( this );
    m_separatorLine->setFixedHeight( 1 );
    m_separatorLine->setContentsMargins( 0, 0, 0, 0 );
    m_separatorLine->setStyleSheet( "QWidget { border-top: 1px solid " +
                                    TomahawkUtils::Colors::BORDER_LINE.name() + "; }" );
    m_layout->addWidget( m_separatorLine );
    m_layout->addWidget( m_label );
    m_layout->addWidget( m_buttons );
    setContentsMargins( contentsMargins().left() + 12,
                        contentsMargins().top() + 8,
                        contentsMargins().right() + 8,
                        contentsMargins().bottom() + 8 );

    m_title->setVisible( false );
    m_separatorLine->setVisible( false );

/*
    m_buttons->button( QDialogButtonBox::Ok )->setStyleSheet(
                       "QPushButton { \
                            background-color: #F15C5E; \
                            border-style: solid; \
                            border-width: 1px; \
                            border-radius: 10px; \
                            border-color: #B64547; \
                            padding: 2px; \
                        } \
                        QPushButton:pressed { \
                            border-style: solid; \
                            border-width: 1px; \
                            border-radius: 10px; \
                            border-color: #B64547; \
                            background-color: #D35052; \
                            border-style: flat; \
                        }" );*/
    setFixedHeight( 80 );
}

void
SourceTreePopupDialog::setTitle( const QString& text )
{
    m_title->setText( text.toUpper() );
    if ( m_title->text().isEmpty() )
    {
        m_title->setVisible( false );
        m_separatorLine->setVisible( false );
    }
    else
    {
        m_title->setVisible( true );
        m_separatorLine->setVisible( true );
    }
}


void
SourceTreePopupDialog::setMainText( const QString& text )
{
    m_label->setText( text );
    QFontMetrics fm = m_label->fontMetrics();
    setFixedWidth( fm.width( text ) + 20 );
}


void
SourceTreePopupDialog::setOkButtonText( const QString& text )
{
    if ( m_buttons && m_buttons->button( QDialogButtonBox::Ok ) )
        m_buttons->button( QDialogButtonBox::Ok )->setText( text );
}


void
SourceTreePopupDialog::setExtraQuestions( const Tomahawk::PlaylistDeleteQuestions& questions )
{
    m_questions = questions;

    int baseHeight = 80;
    int idx = m_layout->indexOf( m_label ) + 1;
    foreach ( const Tomahawk::PlaylistDeleteQuestion& question, m_questions )
    {
        QCheckBox* cb = new QCheckBox( question.first, this );
        cb->setLayoutDirection( Qt::RightToLeft );
        cb->setProperty( "data", question.second );

        QHBoxLayout* h = new QHBoxLayout;
        h->addStretch( 1 );
        h->addWidget( cb );
//         m_layout->insertLayout( h, cb, 0 );
        m_layout->insertLayout( idx, h, 0 );

        m_questionCheckboxes << cb;
        idx++;
        baseHeight += cb->height() + m_layout->spacing();
    }
    setFixedHeight( baseHeight );
}


void
SourceTreePopupDialog::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );

    // Constants for painting
    const int leftTriangleWidth = 12;
    const int cornerRounding = TomahawkUtils::POPUP_ROUNDING_RADIUS;
    const int leftEdgeOffset = 2 /*margin*/ + leftTriangleWidth / 2;
    const QRect brect = rect().adjusted( 2, 3, -2, -3 );

    QPainterPath outline;

    // Main rect
    outline.addRoundedRect( brect.adjusted( leftTriangleWidth / 2, 0, 0, 0 ), cornerRounding, cornerRounding );

    // Left triangle top branch
    outline.moveTo( brect.left(), brect.top() + brect.height() / 2 );
    outline.lineTo( leftEdgeOffset, brect.top() + brect.height() / 2 - leftTriangleWidth / 2 );

    // Left triangle bottom branch
    outline.lineTo( leftEdgeOffset, brect.top() + brect.height() / 2 + leftTriangleWidth / 2 );
    outline.lineTo( brect.left(), brect.top() + brect.height() / 2 );

#ifndef Q_OS_MAC
    TomahawkUtils::drawCompositedPopup( this,
                                        outline,
                                        TomahawkUtils::Colors::BORDER_LINE,
                                        TomahawkUtils::Colors::POPUP_BACKGROUND,
                                        TomahawkUtils::POPUP_OPACITY );
#else
    TomahawkUtils::drawCompositedPopup( this,
                                        outline,
                                        TomahawkUtils::Colors::BORDER_LINE,
                                        QColor( "#D6E3F1" ),
                                        0.93 );
#endif
}


void
SourceTreePopupDialog::focusOutEvent( QFocusEvent* )
{
    hide();
}


void
SourceTreePopupDialog::showEvent( QShowEvent* )
{
    m_result = false;
}


void
SourceTreePopupDialog::onAccepted()
{
    hide();
    m_result = true;
    calculateResults();
    emit result( m_result );
}


void
SourceTreePopupDialog::onRejected()
{
    hide();
    m_result = false;
    calculateResults();
    emit result( m_result );
}


void
SourceTreePopupDialog::calculateResults()
{
    foreach ( const QCheckBox* b, m_questionCheckboxes )
    {
        if ( b->property( "data" ).toInt() != 0 )
        {
            m_questionResults[ b->property( "data" ).toInt() ] = ( b->checkState() == Qt::Checked );
        }
    }
}
