#include "proxystyle.h"

#include <QPainter>
#include <QStyleOption>
#include <QWidget>


void
ProxyStyle::drawPrimitive( PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* w ) const
{
    if ( pe != PE_FrameStatusBar )
        QProxyStyle::drawPrimitive( pe, opt, p, w );
}
