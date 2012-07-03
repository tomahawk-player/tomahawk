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

#ifndef DYNAMIC_QML_WIDGET_H
#define DYNAMIC_QML_WIDGET_H

#include "ViewPage.h"
#include "Typedefs.h"

#include <QDeclarativeView>

namespace Tomahawk
{


class DynamicQmlWidget : public QDeclarativeView, public Tomahawk::ViewPage
{
Q_OBJECT
public:
    explicit DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent = 0);
    virtual ~DynamicQmlWidget();

};

}
#endif // DYNAMIC_QML_WIDGET_H
