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

#ifndef MPRIS_PLUGIN_ROOT_ADAPTOR
#define MPRIS_PLUGIN_ROOT_ADAPTOR

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

#include <QStringList>

class MprisPluginRootAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_CLASSINFO("D-Bus Introspection", ""
		"  <interface name=\"org.mpris.MediaPlayer2\">\n"
		"    <method name=\"Raise\"/>\n"
		"    <method name=\"Quit\"/>\n"
		"    <property name=\"CanQuit\" type=\"b\" access=\"read\"/>\n"
		"    <property name=\"CanRaise\" type=\"b\" access=\"read\"/>\n"
		"    <property name=\"HasTrackList\" type=\"b\" access=\"read\"/>\n"
		"    <property name=\"Identity\" type=\"s\" access=\"read\"/>\n"
		"    <property name=\"DesktopEntry\" type=\"s\" access=\"read\"/>\n"
		"    <property name=\"SupportedUriSchemes\" type=\"as\" access=\"read\"/>\n"
		"    <property name=\"SupportedMimeTypes\" type=\"as\" access=\"read\"/>\n"
		"  </interface>\n"
		"")
    Q_PROPERTY( bool CanQuit READ canQuit )
    Q_PROPERTY( bool CanRaise READ canRaise )
    Q_PROPERTY( bool HasTrackList READ hasTrackList )
    Q_PROPERTY( QString Identity READ identity )
    Q_PROPERTY( QString DesktopEntry READ desktopEntry )
    Q_PROPERTY( QStringList SupportedUriSchemes READ supportedUriSchemes )
    Q_PROPERTY( QStringList SupportedMimeTypes READ supportedMimeTypes )


public:
    MprisPluginRootAdaptor( QObject *parent );
    virtual ~MprisPluginRootAdaptor();

    bool canQuit();
    bool canRaise();
    bool hasTrackList();
    QString identity();
    QString desktopEntry();
    QStringList supportedUriSchemes();
    QStringList supportedMimeTypes();


public slots:
    Q_NOREPLY void Raise();
    Q_NOREPLY void Quit();

};


#endif
