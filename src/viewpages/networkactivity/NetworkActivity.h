/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef NETWORKACTIVITY_H
#define NETWORKACTIVITY_H

#include "NetworkActivityWidget.h"

#include "ViewPage.h"
#include "ViewPageLazyLoader.h"

#include "../ViewPageDllMacro.h"

class AnimatedSpinner;
class PlaylistModel;
class QModelIndex;
class QStandardItemModel;
class QSortFilterProxyModel;
namespace Ui
{
    class NetworkActivityWidget;
}

namespace Tomahawk
{
namespace Widgets
{

class NetworkActivityWidgetPrivate;

class TOMAHAWK_VIEWPAGE_EXPORT NetworkActivity : public Tomahawk::ViewPageLazyLoader< NetworkActivityWidget >
{
Q_OBJECT
Q_INTERFACES( Tomahawk::ViewPagePlugin )
Q_PLUGIN_METADATA( IID "org.tomahawk-player.Player.ViewPagePlugin" )

public:
    NetworkActivity() {}
    virtual ~NetworkActivity() {}

    virtual const QString defaultName() { return QLatin1String( "networkactivity" ); }
    virtual QString title() const { return tr( "Trending" ); }
    virtual QString description() const { return tr( "What's hot amongst your friends" ); }
    virtual const QString pixmapPath() const { return ( RESPATH "images/trending.svg" ); }
    virtual bool showInfoBar() const { return true; }
    
    virtual int sortValue() { return 2; }
};

} // Widgets

} // Tomahawk

#endif // NETWORKACTIVITY_H
