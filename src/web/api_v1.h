#ifndef TOMAHAWK_WEBAPI_V1
#define TOMAHAWK_WEBAPI_V1

// See: http://doc.libqxt.org/tip/qxtweb.html

#include "query.h"
#include "pipeline.h"

#include "QxtHttpServerConnector"
#include "QxtHttpSessionManager"
#include "QxtWebSlotService"
#include "QxtWebPageEvent"

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include <QFile>
#include <QSharedPointer>
#include <QStringList>

#include "network/servent.h"
#include "tomahawkutils.h"
#include "tomahawk/tomahawkapp.h"
#include <database/databasecommand_addclientauth.h>
#include <qxtwebcontent.h>
#include <database/database.h>
#include <database/databasecommand_clientauthvalid.h>

class Api_v1 : public QxtWebSlotService
{
Q_OBJECT

public:

    Api_v1( QxtAbstractWebSessionManager* sm, QObject* parent = 0 )
        : QxtWebSlotService( sm, parent )
    {
    }

public slots:
    // authenticating uses /auth_1
    // we redirect to /auth_2 for the callback
    void auth_1( QxtWebRequestEvent* event );
    void auth_2( QxtWebRequestEvent* event );

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

