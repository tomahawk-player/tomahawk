/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "KdeTelepathyConfigWidget.h"

#include "utils/Logger.h"

#include <KCModuleProxy>

#include <QtPlugin>

QWidget*
KdeTelepathyConfigWidget::configWidget()
{
    KCModuleProxy* proxy = new KCModuleProxy( "kcm_ktp_accounts" );

    if ( !proxy->aboutData() )
    {
        qWarning() << "Could not load kcm_ktp_accounts... ";

        delete proxy;

        return 0;
    }

   return proxy;
}

Q_EXPORT_PLUGIN2( TelepathyConfigStorageConfigWidgetPlugin, KdeTelepathyConfigWidget )


