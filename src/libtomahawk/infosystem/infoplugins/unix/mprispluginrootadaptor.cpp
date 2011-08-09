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

#include "mprispluginrootadaptor.h"

MprisPluginRootAdaptor::MprisPluginRootAdaptor( QObject *parent )
  :  QDBusAbstractAdaptor( parent )
{
}

MprisPluginRootAdaptor::~MprisPluginRootAdaptor()
{
}

// Properties

bool
MprisPluginRootAdaptor::canQuit()
{
    qDebug() << Q_FUNC_INFO;
    bool retVal;
    QMetaObject::invokeMethod(parent()
			      , "canQuit"
			      , Qt::DirectConnection
			      ,  Q_RETURN_ARG( bool, retVal ) );
    return retVal;
}

bool
MprisPluginRootAdaptor::canRaise()
{
    qDebug() << Q_FUNC_INFO;
    bool retVal;
    QMetaObject::invokeMethod(parent()
			      , "canRaise"
			      , Qt::DirectConnection
			      ,  Q_RETURN_ARG( bool, retVal ) );
    return retVal;
}

bool 
MprisPluginRootAdaptor::hasTrackList()
{
    qDebug() << Q_FUNC_INFO;
    bool retVal;
    QMetaObject::invokeMethod(parent()
			      , "hasTrackList"
			      , Qt::DirectConnection
			      ,  Q_RETURN_ARG( bool, retVal ) );
    return retVal;
}

QString
MprisPluginRootAdaptor::identity()
{
    qDebug() << Q_FUNC_INFO;
    QString retVal;
    QMetaObject::invokeMethod(parent()
			      , "identity"
			      , Qt::DirectConnection
			      ,  Q_RETURN_ARG( QString, retVal ) );
    return retVal;
}

QString 
MprisPluginRootAdaptor::desktopEntry()
{
    qDebug() << Q_FUNC_INFO;
    QString retVal;
    QMetaObject::invokeMethod(parent()
			      , "desktopEntry"
			      , Qt::DirectConnection
			      ,  Q_RETURN_ARG( QString, retVal ) );
    return retVal;
}

QStringList
MprisPluginRootAdaptor::supportedUriSchemes()
{
    qDebug() << Q_FUNC_INFO;
    QStringList retVal;
    QMetaObject::invokeMethod(parent()
			      , "supportedUriSchemes"
			      , Qt::DirectConnection
			      ,  Q_RETURN_ARG( QStringList, retVal ) );
    return retVal;
}

QStringList
MprisPluginRootAdaptor::supportedMimeTypes()
{
    qDebug() << Q_FUNC_INFO;
    QStringList retVal;
    QMetaObject::invokeMethod(parent()
			      , "supportedMimeTypes"
			      , Qt::DirectConnection
			      ,  Q_RETURN_ARG( QStringList, retVal ) );
    return retVal;
}

// Methods

void
MprisPluginRootAdaptor::Raise()
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(parent(), "raise");
}

void
MprisPluginRootAdaptor::Quit()
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(parent(), "quit");
}
