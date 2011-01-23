#include "overlaywidget.h"

#include <QPainter>


OverlayWidget::OverlayWidget()
    : QWidget()
{
    resize( 380, 220 );
    setAttribute( Qt::WA_TranslucentBackground, true );
}


OverlayWidget::~OverlayWidget()
{
}


void
OverlayWidget::setText( const QString& text )
{
    if ( text == m_text )
        return;

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
    pixmap(); // cache the image

    QRect center( QPoint( ( painter->viewport().width() - m_pixmap.width() ) / 2,
                          ( painter->viewport().height() - m_pixmap.height() ) / 2 ), m_pixmap.size() );

    painter->drawPixmap( center, m_pixmap );
}


void
OverlayWidget::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    QRect r = contentsRect();

    p.save();
    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );

    p.setPen( palette().shadow().color() );
    p.setBrush( palette().shadow() );
    p.setOpacity( 0.7 );

    p.drawRoundedRect( r, 32.0, 32.0 );

    QTextOption to( Qt::AlignCenter );
    to.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );

    QFont f( font() );
    f.setPixelSize( 18 );
    f.setBold( true );

    p.setFont( f );
    p.setPen( palette().light().color() );
    p.drawText( r.adjusted( 16, 16, -16, -16 ), text(), to );

    p.restore();
}
