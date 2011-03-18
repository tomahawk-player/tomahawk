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

#ifndef TOMAHAWK_PLUGIN_H
#define TOMAHAWK_PLUGIN_H

#include <QString>
#include <QtPlugin>

#include "pluginapi.h"

class TomahawkPlugin
{
public:
    TomahawkPlugin(){};
    TomahawkPlugin(Tomahawk::PluginAPI * api)
        : m_api(api) {};

    virtual TomahawkPlugin * factory(Tomahawk::PluginAPI * api) = 0;

    virtual QString name() const = 0;
    virtual QString description() const = 0;

protected:
    Tomahawk::PluginAPI * api() const { return m_api; };

private:
    Tomahawk::PluginAPI * m_api;

};

Q_DECLARE_INTERFACE(TomahawkPlugin, "org.tomahawk.TomahawkPlugin/1.0")

#endif
