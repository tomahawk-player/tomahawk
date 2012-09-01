/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ContextPage.h"

#include <QGraphicsLinearLayout>

#include "PlaylistInterface.h"
#include "utils/StyleHelper.h"
#include "utils/TomahawkUtilsGui.h"

using namespace Tomahawk;


void
ContextProxyPage::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    painter->save();

    painter->setRenderHint( QPainter::Antialiasing, true );
    painter->setPen( StyleHelper::headerHighlightColor() );
    painter->setBrush( StyleHelper::headerHighlightColor() );
    painter->drawRoundedRect( option->rect, 4.0, 4.0 );

    QFont f( font() );
    f.setBold( true );
    f.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
    painter->setFont( f );
    painter->setPen( Qt::white );

    QFontMetrics fm( f );
    QRect r( 1, 1, option->rect.width(), fm.height() * 1.1 );
    QTextOption to( Qt::AlignCenter );
    painter->drawText( r, m_page->title(), to );

    painter->restore();

    QGraphicsWidget::paint( painter, option, widget );
}


void
ContextProxyPage::setPage( Tomahawk::ContextPage* page )
{
    m_page = page;

#ifdef Q_WS_X11 //FIXME: why do we need this? maybe it's only oxygen style misbehaving?
    QGraphicsWebView* testWebView = qobject_cast<QGraphicsWebView*>( page->widget() );
    if ( testWebView )
    {
        setContentsMargins( 4, 4, 4, 4 );
    }
#endif

    QFont f( font() );
    f.setBold( true );
    f.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
    QFontMetrics fm( f );
    QGraphicsLinearLayout* layout = new QGraphicsLinearLayout();
    layout->setContentsMargins( 4, fm.height() * 1.1, 4, 4 );
    layout->addItem( page->widget() );
    setLayout( layout );

    page->widget()->installEventFilter( this );
}


bool
ContextProxyPage::eventFilter( QObject* watched, QEvent* event )
{
    if ( event->type() == QEvent::GrabMouse )
    {
        emit focused();
    }

    return QGraphicsWidget::eventFilter( watched, event );
}


bool
ContextProxyPage::sceneEvent( QEvent* event )
{
    if ( event->type() == QEvent::GrabMouse )
    {
        emit focused();
    }

    return QGraphicsWidget::sceneEvent( event );
}
