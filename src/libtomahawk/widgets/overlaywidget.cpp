#include "overlaywidget.h"

#include <QDebug>
#include <QPainter>
#include <QPropertyAnimation>

#define CORNER_ROUNDNESS 16.0
#define FADEIN_DURATION 500
#define FONT_SIZE 18
#define OPACITY 0.80


OverlayWidget::OverlayWidget( QAbstractItemView* parent )
    : QWidget() // this is on purpose!
    , m_parent( parent )
{
    resize( 380, 220 );
    setAttribute( Qt::WA_TranslucentBackground, true );
}


OverlayWidget::~OverlayWidget()
{
}


void
OverlayWidget::setOpacity( qreal opacity )
{
    m_opacity = opacity;
    m_parent->reset();
}


void
OverlayWidget::setText( const QString& text )
{
    if ( text == m_text )
        return;

    if ( isEnabled() )
    {
        QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
        animation->setDuration( FADEIN_DURATION );
        animation->setStartValue( 0.00 );
        animation->setEndValue( OPACITY );
        animation->start();
    }
    else
        m_opacity = OPACITY;

    m_text = text;
    m_pixmap = QPixmap();
}


QPixmap
OverlayWidget::pixmap()
{
    if ( m_pixmap.isNull() )
    {
        QPixmap p( contentsRect().size() );
        p.fill( Qt::transparent );
        render( &p );

        m_pixmap = p;
    }

    return m_pixmap;
}


void
OverlayWidget::paint( QPainter* painter )
{
    if ( !isEnabled() )
        return;

    pixmap(); // cache the image

    QRect center( QPoint( ( painter->viewport().width() - m_pixmap.width() ) / 2,
                          ( painter->viewport().height() - m_pixmap.height() ) / 2 ), m_pixmap.size() );

    painter->save();
    painter->setOpacity( m_opacity );
    painter->drawPixmap( center, m_pixmap );
    painter->restore();
}


void
OverlayWidget::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    QRect r = contentsRect();

    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );

    QPen pen( palette().dark().color(), .5 );
    p.setPen( pen );
    p.setBrush( palette().highlight() );

    p.drawRoundedRect( r, CORNER_ROUNDNESS, CORNER_ROUNDNESS );

    QTextOption to( Qt::AlignCenter );
    to.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );

    QFont f( font() );
    f.setPixelSize( FONT_SIZE );
    f.setBold( true );

    p.setFont( f );
    p.setPen( palette().highlightedText().color() );
    p.drawText( r.adjusted( 16, 16, -16, -16 ), text(), to );
}
