/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef TOMAHAWK_WEBAPI_V1
#define TOMAHAWK_WEBAPI_V1

// See: http://doc.libqxt.org/tip/qxtweb.html

#include <QxtWeb/QxtHttpServerConnector>
#include <QxtWeb/QxtHttpSessionManager>
#include <QxtWeb/QxtWebContent>
#include <QxtWeb/QxtWebSlotService>
#include <QxtWeb/QxtWebPageEvent>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include <QFile>
#include <QSharedPointer>
#include <QStringList>

class Api_v1 : public QxtWebSlotService
{
Q_OBJECT

public:

    Api_v1( QxtAbstractWebSessionManager* sm, QObject* parent = 0 );

public slots:
    // authenticating uses /auth_1
    // we redirect to /auth_2 for the callback
    void auth_1( QxtWebRequestEvent* event, QString unused = QString() );
    void auth_2( QxtWebRequestEvent* event, QString unused = QString() );

    // all v1 api calls go to /api/
    void api( QxtWebRequestEvent* event );

    // request for stream: /sid/<id>
    void sid( QxtWebRequestEvent* event, QString unused = QString() );
    void send404( QxtWebRequestEvent* event );
    void stat( QxtWebRequestEvent* event );
    void statResult( const QString& clientToken, const QString& name, bool valid );
    void resolve( QxtWebRequestEvent* event );
    void staticdata( QxtWebRequestEvent* event,const QString& );
    void get_results( QxtWebRequestEvent* event );
    void sendJSON( const QVariantMap& m, QxtWebRequestEvent* event );

    // load an html template from a file, replace args from map
    // then serve
    void sendWebpageWithArgs( QxtWebRequestEvent* event, const QString& filenameSource, const QHash< QString, QString >& args );

    void index( QxtWebRequestEvent* event );

private:
    QxtWebRequestEvent* m_storedEvent;
};

#endif

