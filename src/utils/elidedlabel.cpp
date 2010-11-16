#include "elidedlabel.h"

#include <QTime>
#include <QEvent>
#include <QPainter>
#include <QFontMetrics>
#include <QApplication>


ElidedLabel::ElidedLabel( QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
{
    init();
}


ElidedLabel::ElidedLabel( const QString& text, QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
{
    init(text);
}


ElidedLabel::~ElidedLabel()
{
}


QString
ElidedLabel::text() const
{
    return m_text;
}


void
ElidedLabel::setText( const QString& text )
{
    if ( m_text != text )
    {
        m_text = text;
        updateLabel();
        emit textChanged( text );
    }
}


Qt::Alignment
ElidedLabel::alignment() const
{
    return align;
}


void
ElidedLabel::setAlignment( Qt::Alignment alignment )
{
    if ( this->align != alignment )
    {
        this->align = alignment;
        update(); // no geometry change, repaint is sufficient
    }
}


Qt::TextElideMode
ElidedLabel::elideMode() const
{
    return mode;
}


void
ElidedLabel::setElideMode( Qt::TextElideMode mode )
{
    if ( this->mode != mode )
    {
        this->mode = mode;
        updateLabel();
    }
}


void
ElidedLabel::init( const QString& txt )
{
    m_text = txt;
    align = Qt::AlignLeft;
    mode = Qt::ElideMiddle;
}


void
ElidedLabel::updateLabel()
{
    updateGeometry();
    update();
}


QSize
ElidedLabel::sizeHint() const
{
    const QFontMetrics& fm = fontMetrics();
    QSize size( fm.width( m_text ), fm.height() );
    return size;
}


QSize
ElidedLabel::minimumSizeHint() const
{
    switch ( mode )
    {
        case Qt::ElideNone:
            return sizeHint();

        default:
        {
            const QFontMetrics& fm = fontMetrics();
            QSize size( fm.width( "..." ), fm.height() );
            return size;
        }
    }
}


void
ElidedLabel::paintEvent( QPaintEvent* event )
{
    QFrame::paintEvent( event );
    QPainter p( this );
    QRect r = contentsRect();
    const QString elidedText = fontMetrics().elidedText( m_text, mode, r.width() );
    p.drawText( r, align, elidedText );
}


void
ElidedLabel::changeEvent( QEvent* event )
{
    QFrame::changeEvent( event );
    switch ( event->type() )
    {
        case QEvent::FontChange:
        case QEvent::ApplicationFontChange:
            updateLabel();
            break;

        default:
            // nothing to do
            break;
    }
}


void
ElidedLabel::mousePressEvent( QMouseEvent* event )
{
    QFrame::mousePressEvent( event );
    time.start();
}


void
ElidedLabel::mouseReleaseEvent( QMouseEvent* event )
{
    QFrame::mouseReleaseEvent( event );
    if ( time.elapsed() < qApp->doubleClickInterval() )
        emit clicked();
}
