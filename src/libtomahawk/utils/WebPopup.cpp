/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2014, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "WebPopup.h"

#include <QDesktopServices>


bool
ExternalBrowserWebPage::acceptNavigationRequest( QWebFrame*, const QNetworkRequest& request, NavigationType )
{
    QDesktopServices::openUrl( request.url() );
    return false;
}


WebPopup::WebPopup( const QUrl& url, const QSize& size )
    : QWebView( 0 )
{
    setAttribute( Qt::WA_DeleteOnClose, true );
    settings()->setAttribute( QWebSettings::DeveloperExtrasEnabled, true );
    settings()->setAttribute( QWebSettings::JavascriptCanCloseWindows, true );
    page()->mainFrame()->setScrollBarPolicy( Qt::Vertical, Qt::ScrollBarAsNeeded );
    page()->mainFrame()->setScrollBarPolicy( Qt::Horizontal, Qt::ScrollBarAsNeeded );

    connect( page(), SIGNAL( windowCloseRequested() ), SLOT( close() ) );

    if ( !size.isEmpty() )
    {
        resize( size );
    }

    load( url );
    show();
}


QWebView*
WebPopup::createWindow( QWebPage::WebWindowType )
{
    QWebView* fakeWindow = new QWebView();
    fakeWindow->setPage( new ExternalBrowserWebPage( fakeWindow ) );
    fakeWindow->deleteLater();
    return fakeWindow;
}
