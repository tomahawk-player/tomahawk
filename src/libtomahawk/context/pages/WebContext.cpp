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

#include "WebContext.h"
#include "Source.h"

using namespace Tomahawk;


WebContext::WebContext()
    : ContextPage()
{
    m_webView = new QGraphicsWebView();

    QPalette pal = m_webView->palette();
    pal.setColor( QPalette::Window, QColor( 0, 0, 0, 0 ) );
    m_webView->setPalette( pal );
}


WebContext::~WebContext()
{
}
