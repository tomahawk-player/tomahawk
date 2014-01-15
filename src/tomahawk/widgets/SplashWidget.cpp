/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,      Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SplashWidget.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/ImageRegistry.h"
#include "utils/DpiScaler.h"

#include <QBoxLayout>
#include <QLabel>

SplashWidget::SplashWidget()
    : QSplashScreen()
{
    setPixmap( ImageRegistry::instance()->pixmap( RESPATH "images/splash.svg",
        TomahawkUtils::DpiScaler::scaled( this, QSize( 304, 333 ) ) ) );

    QFont font = this->font();

    font.setPointSize( 9 );
    font.setBold( true );
    font.setFamily( "Titillium Web" );
    setFont( font );
}


void
SplashWidget::showMessage( const QString& message )
{
    QSplashScreen::showMessage( message + "\n\n", Qt::AlignBottom | Qt::AlignHCenter );
}
