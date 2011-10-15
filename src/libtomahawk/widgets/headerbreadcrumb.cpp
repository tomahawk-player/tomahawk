/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Casey Link <unnamedrambler@gmail.com>
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

#include "headerbreadcrumb.h"

#include "utils/stylehelper.h"

#include <QStyle>
#include <QStylePainter>
#include <QPaintEvent>

HeaderBreadCrumb::HeaderBreadCrumb(BreadcrumbButtonFactory *buttonFactory, QWidget *parent) :
    BreadcrumbBar(buttonFactory, parent)
{
}

HeaderBreadCrumb::HeaderBreadCrumb(QWidget *parent) :
    BreadcrumbBar(parent)
{
}

HeaderBreadCrumb::~HeaderBreadCrumb()
{
}

void HeaderBreadCrumb::paintEvent(QPaintEvent *event)
{
    QStylePainter p(this);
    StyleHelper::horizontalHeader(&p, rect());
}
