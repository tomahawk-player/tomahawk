#include "imagebutton.h"

#include <QPainter>
#include <QPaintEvent>
#include <QLayout>
#include <QAction>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QDebug>


ImageButton::ImageButton( QWidget* parent )
    : QAbstractButton( parent )
{
}


ImageButton::ImageButton( const QPixmap& rest, QWidget* parent )
    : QAbstractButton( parent )
{
    init( rest );
}


ImageButton::ImageButton( const QString& path, QWidget* parent )
    : QAbstractButton( parent )
{
    init( QPixmap( path ) );
}


void
ImageButton::init( const QPixmap& p )
{
    setPixmap( p, QIcon::Off );
    m_sizeHint = p.size();
    updateGeometry();
}


void
ImageButton::setPixmap( const QString& path )
{
    init( QPixmap( path ) );
}


void
ImageButton::setPixmap( const QPixmap& pixmap )
{
    init( pixmap );
}


void
ImageButton::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    p.setClipRect( event->rect() );
    
    QIcon::Mode mode = isDown()
        ? QIcon::Active
        : isEnabled() 
            ? QIcon::Normal 
            : QIcon::Disabled;
    
    QIcon::State state = isChecked()
        ? QIcon::On 
        : QIcon::Off;
    
    icon().paint( &p, rect(), Qt::AlignCenter, mode, state );
}


void
ImageButton::setPixmap( const QString& path, const QIcon::State state, const QIcon::Mode mode )
{      
    setPixmap( QPixmap( path ), state, mode );
}


void
ImageButton::setPixmap( const QPixmap& p, const QIcon::State state, const QIcon::Mode mode )
{
    QIcon i = icon();
    i.addPixmap( p, mode, state );
    setIcon( i );
}
