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

#ifndef WEBPOPUP_H
#define WEBPOPUP_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <QWebView>
#include <QWebFrame>

class ExternalBrowserWebPage : public QWebPage
{
    Q_OBJECT
public:
    ExternalBrowserWebPage( QObject* parent )
    : QWebPage(parent)
    {
    }

protected:
    bool acceptNavigationRequest( QWebFrame*, const QNetworkRequest& request, NavigationType ) override;
};


class WebPopup : public QWebView
{
    Q_OBJECT

public:
    WebPopup( const QUrl& url, const QSize& size );

    QWebView* createWindow( QWebPage::WebWindowType );
};

#endif // WEBPOPUP_H
