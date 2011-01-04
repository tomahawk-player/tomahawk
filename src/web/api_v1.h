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

#include "network/servent.h"

class Api_v1 : public QxtWebSlotService
{
Q_OBJECT

public:

    Api_v1(QxtAbstractWebSessionManager * sm, QObject * parent = 0 )
        : QxtWebSlotService(sm, parent)
    {
    }

public slots:

    // all v1 api calls go to /api/
    void api(QxtWebRequestEvent* event)
    {
        qDebug() << "HTTP" << event->url.toString();

        const QUrl& url = event->url;
        if( url.hasQueryItem( "method" ) )
        {
            const QString method = url.queryItemValue( "method" );

            if( method == "stat" )          return stat(event);
            if( method == "resolve" )       return resolve(event);
            if( method == "get_results" )   return get_results(event);
        }

        send404( event );
    }

    // request for stream: /sid/<id>
    void sid(QxtWebRequestEvent* event, QString unused = "")
    {
        using namespace Tomahawk;
        RID rid = event->url.path().mid(5);
        qDebug() << "Request for sid " << rid;
        result_ptr rp = Pipeline::instance()->result( rid );
        if( rp.isNull() )
        {
            return send404( event );
        }
        QSharedPointer<QIODevice> iodev = Servent::instance()->getIODeviceForUrl( rp );
        if( iodev.isNull() )
        {
            return send404( event ); // 503?
        }
        QxtWebPageEvent* e = new QxtWebPageEvent( event->sessionID, event->requestID, iodev );
        e->streaming = iodev->isSequential();
        e->contentType = rp->mimetype().toAscii();
        e->headers.insert("Content-Length", QString::number( rp->size() ) );
        postEvent(e);
        return;
    }

    void send404( QxtWebRequestEvent* event )
    {
        qDebug() << "404" << event->url.toString();
        QxtWebPageEvent* wpe = new QxtWebPageEvent(event->sessionID, event->requestID, "<h1>Not Found</h1>");
        wpe->status = 404;
        wpe->statusMessage = "not found";
        postEvent( wpe );
    }

    void stat( QxtWebRequestEvent* event )
    {
        QVariantMap m;
        m.insert( "name", "playdar" );
        m.insert( "version", "0.1.1" ); // TODO (needs to be >=0.1.1 for JS to work)
        m.insert( "authenticated", true ); // TODO
        m.insert( "capabilities", QVariantList() );
        sendJSON( m, event );
    }

    void resolve( QxtWebRequestEvent* event )
    {
        if( !event->url.hasQueryItem("artist") ||
            !event->url.hasQueryItem("track") )
        {
            qDebug() << "Malformed HTTP resolve request";
            send404(event);
        }
        QString qid;
        if( event->url.hasQueryItem("qid") ) qid = event->url.queryItemValue("qid");
        else qid = uuid();

        QVariantMap m;
        m.insert( "artist", event->url.queryItemValue("artist") );
        m.insert( "album", event->url.queryItemValue("album") );
        m.insert( "track", event->url.queryItemValue("track") );
        m.insert( "qid", qid );

        Tomahawk::query_ptr qry( new Tomahawk::Query( m ) );
        Tomahawk::Pipeline::instance()->add( qry );

        QVariantMap r;
        r.insert( "qid", qid );
        sendJSON( r, event );
    }

    void get_results( QxtWebRequestEvent* event )
    {
        if( !event->url.hasQueryItem("qid") )
        {
            qDebug() << "Malformed HTTP get_results request";
            send404(event);
        }

        using namespace Tomahawk;
        query_ptr qry = Pipeline::instance()->query( event->url.queryItemValue("qid") );
        if( qry.isNull() )
        {
            send404( event );
            return;
        }

        QVariantMap r;
        r.insert( "qid", qry->id() );
        r.insert( "poll_interval", 1000 );
        r.insert( "refresh_interval", 1000 );
        r.insert( "poll_limit", 6 );
        r.insert( "solved", qry->solved() );
        r.insert( "query", qry->toVariant() );
        QVariantList res;
        foreach( Tomahawk::result_ptr rp, qry->results() )
        {
            res << rp->toVariant();
        }
        r.insert( "results", res );

        sendJSON( r, event );
    }

    void sendJSON( const QVariantMap& m, QxtWebRequestEvent* event )
    {
        QJson::Serializer ser;
        QByteArray ctype;
        QByteArray body = ser.serialize( m );
        if( event->url.hasQueryItem("jsonp") && !event->url.queryItemValue( "jsonp" ).isEmpty() )
        {
            ctype = "text/javascript; charset=utf-8";
            body.prepend( QString("%1( ").arg( event->url.queryItemValue( "jsonp" ) ).toAscii() );
            body.append( " );" );
        }
        else
        {
            ctype = "appplication/json; charset=utf-8";
        }
        QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, body );
        e->contentType = ctype;
        e->headers.insert( "Content-Length", QString::number( body.length() ) );
        postEvent( e );
        qDebug() << "JSON response" << event->url.toString() << body;
    }


    void index(QxtWebRequestEvent* event)
    {
        send404( event );
        return;

    }

};

#endif

