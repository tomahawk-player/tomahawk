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

#ifndef VIEWPAGE_H
#define VIEWPAGE_H

#include <QObject>

#include "typedefs.h"
#include "playlistinterface.h"
#include "utils/tomahawkutils.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT ViewPage
{
public:
    ViewPage() {}

    virtual QWidget* widget() = 0;
    virtual PlaylistInterface* playlistInterface() const = 0;

    virtual QString title() const = 0;
    virtual QString description() const = 0;
    virtual QPixmap pixmap() const { return QPixmap( RESPATH "icons/tomahawk-icon-128x128.png" ); }

    virtual bool showStatsBar() const { return true; }
    virtual bool showModes() const { return false; }
    virtual bool queueVisible() const { return true; }

    virtual bool jumpToCurrentTrack() = 0;

    /** subclasses implementing ViewPage can emit the following signals:
     * descriptionChanged( const QString& )
     * destroyed( QWidget* widget );
     *
     * See DynamicWidget for an example
     */
private:
};

}; // ns

#endif //VIEWPAGE_H
