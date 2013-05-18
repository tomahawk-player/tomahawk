/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Michael Zanetti <mzanetti@kde.org>
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

#include "DeclarativeView.h"
#include "playlist/PlayableItem.h"
#include "DeclarativeCoverArtProvider.h"
#include "utils/TomahawkUtilsGui.h"

#include <qdeclarative.h>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

namespace Tomahawk
{

DeclarativeView::DeclarativeView( QWidget *parent ):
    QDeclarativeView( parent )
{

    // Needed to make the QML contents scale with tomahawk
    setResizeMode( QDeclarativeView::SizeRootObjectToView );

    // This types seem to be needed everywhere anyways, lets the register here
    qmlRegisterType<PlayableItem>( "tomahawk", 1, 0, "PlayableItem");
//    qmlRegisterType<SearchFieldQmlProxy>("tomahawk", 1, 0, "SearchField");

    // QML image providers will be deleted by the view
    engine()->addImageProvider( "albumart", new DeclarativeCoverArtProvider() );

    // Register the view itself to make it easy to invoke the view's slots from QML
    rootContext()->setContextProperty( "mainView", this );

    rootContext()->setContextProperty( "defaultFontSize", TomahawkUtils::defaultFontSize() );
    rootContext()->setContextProperty( "defaultFontHeight", TomahawkUtils::defaultFontHeight() );

}

DeclarativeView::~DeclarativeView()
{

}

}
