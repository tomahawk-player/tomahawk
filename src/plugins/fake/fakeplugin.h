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

#ifndef TOMAHAWK_LIB_PLUGIN_H
#define TOMAHAWK_LIB_PLUGIN_H
#include <QtPlugin>

#include "tomahawk/plugin_includes.h"

class FakePlugin : public QObject, public TomahawkPlugin
{
    Q_OBJECT
    Q_INTERFACES(TomahawkPlugin)

public:

    FakePlugin(){};

    FakePlugin(Tomahawk::PluginAPI* api);
    TomahawkPlugin * factory(Tomahawk::PluginAPI* api);
    QString name() const { return "FakePlugin"; };
    QString description() const { return "Fake stuff, hardcoded"; };

private:

    void init();
    
    Tomahawk::PluginAPI* m_api;
};



#endif

