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

#include "OverlayWidget.h"

#include "playlist/PlayableProxyModel.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QPainter>
#include <QPropertyAnimation>


#define CORNER_ROUNDNESS 8.0
#define FADING_DURATION 500
#define OPACITY 0.70


OverlayWidget::OverlayWidget( QWidget* parent )
    : QWidget( parent ) // this is on purpose!
    , m_parent( parent )
    , m_itemView( 0 )
{
    init();
}


OverlayWidget::OverlayWidget( QAbstractItemView* parent )
    : QWidget( parent ) // this is on purpose!
    , m_parent( parent )
    , m_itemView( parent )
{
    init();

    if ( m_itemView->model() )
    {
        connect( m_itemView->model(), SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onViewChanged() ), Qt::UniqueConnection );
        connect( m_itemView->model(), SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( onViewChanged() ), Qt::UniqueConnection );
        connect( m_itemView->model(), SIGNAL( loadingStarted() ), SLOT( onViewChanged() ), Qt::UniqueConnection );
        connect( m_itemView->model(), SIGNAL( loadingFinished() ), SLOT( onViewChanged() ), Qt::UniqueConnection );
    }
    connect( m_itemView, SIGNAL( modelChanged() ), SLOT( onViewModelChanged() ) );
}


OverlayWidget::~OverlayWidget()
{
}


void
OverlayWidget::init()
{
    installEventFilter( m_parent );
    setAcceptDrops( true );

    setAttribute( Qt::WA_TranslucentBackground, true );
    m_opacity = 0.00;
    setOpacity( m_opacity );

    m_timer.setSingleShot( true );
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( hide() ) );
}


void
OverlayWidget::setOpacity( qreal opacity )
{
    m_opacity = opacity;

    if ( m_opacity == 0.00 && !isHidden() )
    {
        QWidget::hide();
    }
    else if ( m_opacity > 0.00 && isHidden() )
    {
        QWidget::show();
    }

    repaint();
}


void
OverlayWidget::setText( const QString& text )
{
    m_text = text;
    onViewChanged();
}


void
OverlayWidget::show( int timeoutSecs )
{
    if ( !isEnabled() )
        return;

    QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
    animation->setDuration( FADING_DURATION );
    animation->setEndValue( 1.0 );
    animation->start();

    if ( timeoutSecs > 0 )
        m_timer.start( timeoutSecs * 1000 );
}


void
OverlayWidget::hide()
{
    if ( !isEnabled() )
        return;

    QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
    animation->setDuration( FADING_DURATION );
    animation->setEndValue( 0.00 );
    animation->start();
}


bool
OverlayWidget::shown() const
{
    if ( !isEnabled() )
        return false;

    return m_opacity == OPACITY;
}


void
OverlayWidget::onViewChanged()
{
    if ( !m_itemView )
        return;

    PlayableProxyModel* model = qobject_cast<PlayableProxyModel*>( m_itemView->model() );
    if ( !model )
        return;

    if ( m_text.isEmpty() || model->rowCount( QModelIndex() ) || model->isLoading() )
    {
        hide();
    }
    else
    {
        show();
    }
}


void
OverlayWidget::onViewModelChanged()
{
    if ( !m_itemView )
        return;

    if ( m_itemView->model() )
    {
        connect( m_itemView->model(), SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onViewChanged() ), Qt::UniqueConnection );
        connect( m_itemView->model(), SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( onViewChanged() ), Qt::UniqueConnection );
        connect( m_itemView->model(), SIGNAL( loadingStarted() ), SLOT( onViewChanged() ), Qt::UniqueConnection );
        connect( m_itemView->model(), SIGNAL( loadingFinished() ), SLOT( onViewChanged() ), Qt::UniqueConnection );

        onViewChanged();
    }
}


void
OverlayWidget::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );

    {
        QSize maxiSize = QSize( (double)m_parent->width() * 0.70, (double)m_parent->height() * 0.70 );
        QSize prefSize = QSize( 380, 128 );
        int width = qMin( maxiSize.width(), prefSize.width() );
        int height = qMin( maxiSize.height(), prefSize.height() );
        QSize newSize = QSize( width, height );

        if ( newSize != size() )
            resize( newSize );
    }

    QPoint center( ( m_parent->width() - width() ) / 2, ( m_parent->height() - height() ) / 2 );
    if ( center != pos() )
    {
        move( center );
        return;
    }

    QPainter p( this );
    QRect r = contentsRect();

    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );
    p.setOpacity( m_opacity );

    QPen pen( palette().dark().color(), .5 );
    p.setPen( pen );
    p.setBrush( QColor( 30, 30, 30, 255.0 * OPACITY ) );

    p.drawRoundedRect( r, CORNER_ROUNDNESS, CORNER_ROUNDNESS );

    QTextOption to( Qt::AlignCenter );
    to.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );

    // shrink to fit if needed
    QFont f( font() );
    f.setPointSize( TomahawkUtils::defaultFontSize() + 7 );
    f.setBold( true );

    QRectF textRect = r.adjusted( 8, 8, -8, -8 );
    qreal availHeight = textRect.height();

    QFontMetricsF fm( f );
    qreal textHeight = fm.boundingRect( textRect, Qt::AlignCenter | Qt::TextWordWrap, text() ).height();
    while( textHeight > availHeight )
    {
        if( f.pointSize() <= 4 ) // don't try harder
            break;

        f.setPointSize( f.pointSize() - 1 );
        fm = QFontMetricsF( f );
        textHeight = fm.boundingRect( textRect, Qt::AlignCenter | Qt::TextWordWrap, text() ).height();
    }

    p.setFont( f );
//    p.setPen( palette().highlightedText().color() );
    p.setPen( Qt::white );
    p.drawText( r.adjusted( 8, 8, -8, -8 ), text(), to );
}
