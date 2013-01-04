/*
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef IMAGE_REGISTRY_H
#define IMAGE_REGISTRY_H

#include <QPixmap>

#include "utils/TomahawkUtilsGui.h"
#include "DllMacro.h"

class DLLEXPORT ImageRegistry
{
public:
    static ImageRegistry* instance();

    explicit ImageRegistry();

    QIcon icon( const QString& image, TomahawkUtils::ImageMode mode = TomahawkUtils::Original );
    QPixmap pixmap( const QString& image, const QSize& size, TomahawkUtils::ImageMode mode = TomahawkUtils::Original );

private:
    void putInCache( const QString& image, const QSize& size, TomahawkUtils::ImageMode mode, const QPixmap& pixmap );

    static ImageRegistry* s_instance;
};

#endif // IMAGE_REGISTRY_H
