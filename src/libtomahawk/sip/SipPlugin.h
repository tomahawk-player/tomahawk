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

#ifndef SIPPLUGIN_H
#define SIPPLUGIN_H

#include <QObject>
#include <QString>
#include <QMenu>

#include "dllmacro.h"

class DLLEXPORT SipPlugin : public QObject
{
    Q_OBJECT

public:
    enum SipErrorCode { AuthError, ConnectionError }; // Placeholder for errors, to be defined

    virtual ~SipPlugin() {}

    virtual bool isValid() = 0;
    virtual const QString name() = 0;
    virtual const QString friendlyName() = 0;
    virtual const QString accountName() = 0;
    virtual QMenu* menu();
    virtual QWidget* configWidget();

public slots:
    virtual bool connectPlugin( bool startup = false ) = 0;
    virtual void disconnectPlugin() = 0;
    virtual void checkSettings() = 0;

    virtual void addContact( const QString &jid, const QString& msg = QString() ) = 0;
    virtual void sendMsg( const QString& to, const QString& msg ) = 0;

signals:
    void error( int, const QString& );
    void connected();
    void disconnected();

    void peerOnline( const QString& );
    void peerOffline( const QString& );
    void msgReceived( const QString& from, const QString& msg );

    void avatarReceived ( const QString& from,  const QPixmap& avatar);

    void addMenu( QMenu* menu );
    void removeMenu( QMenu* menu );
};

Q_DECLARE_INTERFACE( SipPlugin, "tomahawk.Sip/1.0" )

#endif
