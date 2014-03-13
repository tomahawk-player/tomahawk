/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "Api_v1.h"

#include "database/Database.h"
#include "database/DatabaseCommand_AddClientAuth.h"
#include "database/DatabaseCommand_ClientAuthValid.h"
#include "network/Servent.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"

#include "Api_v1_5.h"
#include "Pipeline.h"
#include "Result.h"
#include "Source.h"
#include "UrlHandler.h"

#include <boost/bind.hpp>

#include <QHash>

using namespace Tomahawk;
using namespace TomahawkUtils;

Api_v1::Api_v1( QxtAbstractWebSessionManager* sm, QObject* parent )
    : QxtWebSlotService(sm, parent)
    , m_api_v1_5( new Api_v1_5( this ) )
{
}

Api_v1::~Api_v1()
{
  delete m_api_v1_5;
}

void
Api_v1::auth_1( QxtWebRequestEvent* event, QString arg )
{
    tDebug( LOGVERBOSE ) << "AUTH_1 HTTP" << event->url.toString() << arg;

    if ( !urlHasQueryItem( event->url, "website" ) || !urlHasQueryItem( event->url, "name" ) )
    {
        tDebug( LOGVERBOSE ) << "Malformed HTTP resolve request";
        send404( event );
        return;
    }

    QString formToken = uuid();

    if ( urlHasQueryItem( event->url, "json" ) )
    {
        // JSON response
        QVariantMap m;
        m[ "formtoken" ] = formToken;
        sendJSON( m, event );
    }
    else
    {
        // webpage request
        QString authPage = RESPATH "www/auth.html";
        QHash< QString, QString > args;
        if ( urlHasQueryItem( event->url, "receiverurl" ) )
            args[ "url" ] = urlQueryItemValue( event->url, "receiverurl" ).toUtf8();

        args[ "formtoken" ] = formToken;
        args[ "website" ] = urlQueryItemValue( event->url, "website" ).toUtf8();
        args[ "name" ] = urlQueryItemValue( event->url, "name" ).toUtf8();
        sendWebpageWithArgs( event, authPage, args );
    }
}


void
Api_v1::auth_2( QxtWebRequestEvent* event, QString arg )
{
    tDebug( LOGVERBOSE ) << "AUTH_2 HTTP" << event->url.toString() << arg;
    if ( event->content.isNull() )
    {
        tDebug( LOGVERBOSE ) << "Null content";
        send404( event );
        return;
    }

    QString params = QUrl::fromPercentEncoding( event->content->readAll() );
    params = params.mid( params.indexOf( '?' ) );
    QStringList pieces = params.split( '&' );
    QHash< QString, QString > queryItems;

    foreach ( const QString& part, pieces )
    {
        QStringList keyval = part.split( '=' );
        if ( keyval.size() == 2 )
            queryItems.insert( keyval.first(), keyval.last() );
        else
            tDebug( LOGVERBOSE ) << "Failed parsing url parameters:" << part;
    }

    tDebug( LOGVERBOSE ) << "has query items:" << pieces;
    if ( !params.contains( "website" ) || !params.contains( "name" ) || !params.contains( "formtoken" ) )
    {
        tDebug( LOGVERBOSE ) << "Malformed HTTP resolve request";
        send404( event );
        return;
    }

    QString website = queryItems[ "website" ];
    QString name  = queryItems[ "name" ];
    QByteArray authtoken = uuid().toLatin1();
    tDebug( LOGVERBOSE ) << "HEADERS:" << event->headers;
    if ( !queryItems.contains( "receiverurl" ) || queryItems.value( "receiverurl" ).isEmpty() )
    {
        //no receiver url, so do it ourselves
        if ( queryItems.contains( "json" ) )
        {
            QVariantMap m;
            m[ "authtoken" ] = authtoken;

            sendJSON( m, event );
        }
        else
        {
            QString authPage = RESPATH "www/auth.na.html";
            QHash< QString, QString > args;
            args[ "authcode" ] = authtoken;
            args[ "website" ] = website;
            args[ "name" ] = name;
            sendWebpageWithArgs( event, authPage, args );
        }
    }
    else
    {
        // do what the client wants
        QUrl receiverurl = QUrl( queryItems.value( "receiverurl" ), QUrl::TolerantMode );
        urlAddQueryItem( receiverurl, "authtoken", "#" + authtoken );
        tDebug( LOGVERBOSE ) << "Got receiver url:" << receiverurl.toString();

        QxtWebRedirectEvent* e = new QxtWebRedirectEvent( event->sessionID, event->requestID, receiverurl.toString() );
        postEvent( e );
        // TODO validation of receiverurl?
    }

    DatabaseCommand_AddClientAuth* dbcmd = new DatabaseCommand_AddClientAuth( authtoken, website, name, event->headers.key( "ua" ) );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr(dbcmd) );
}


/**
 * Handle API calls.
 *
 * All v1.0 API (standard Playdar) calls go to /api/?method=
 * All v1.5 API (simple remote control) calls go to /api/1.5/
 */
void
Api_v1::api( QxtWebRequestEvent* event, const QString& version, const QString& method, const QString& arg1, const QString& arg2, const QString& arg3 )
{
    tDebug( LOGVERBOSE ) << "HTTP" << event->url.toString();

    if ( version.isEmpty() ) {
      // We dealing with API 1.0

      const QUrl& url = event->url;
      if ( urlHasQueryItem( url, "method" ) )
      {
          const QString method = urlQueryItemValue( url, "method" );

          if ( method == "stat" )        return stat( event );
          if ( method == "resolve" )     return resolve( event );
          if ( method == "get_results" ) return get_results( event );
      }

      send404( event );
    }
    else if ( version == "1.5" )
    {
        if ( !arg3.isEmpty() )
        {
            if ( !QMetaObject::invokeMethod( m_api_v1_5, method.toLatin1().constData(), Q_ARG( QxtWebRequestEvent*, event ), Q_ARG( QString, arg1 ), Q_ARG( QString, arg2 ), Q_ARG( QString, arg3 ) ) )
            {
                apiCallFailed(event, method);
            }
        }
        else if ( !arg2.isEmpty() )
        {
            if ( !QMetaObject::invokeMethod( m_api_v1_5, method.toLatin1().constData(), Q_ARG( QxtWebRequestEvent*, event ), Q_ARG( QString, arg1 ), Q_ARG( QString, arg2 ) ) )
            {
                apiCallFailed(event, method);
            }
        }
        else if ( !arg1.isEmpty() )
        {
            if ( !QMetaObject::invokeMethod( m_api_v1_5, method.toLatin1().constData(), Q_ARG( QxtWebRequestEvent*, event ), Q_ARG( QString, arg1 ) ) )
            {
                apiCallFailed(event, method);
            }
        }
        else
        {
            if ( !QMetaObject::invokeMethod( m_api_v1_5, method.toLatin1().constData(), Q_ARG( QxtWebRequestEvent*, event ) ) )
            {
                apiCallFailed(event, method);
            }
        }
    }
    else
    {
        sendPlain404( event, QString( "Unknown API version %1" ).arg( version ), "API version not found" );
    }
}


// request for stream: /sid/<id>
void
Api_v1::sid( QxtWebRequestEvent* event, QString unused )
{
    Q_UNUSED( unused );

    RID rid = event->url.path().mid( 5 );
    tDebug( LOGVERBOSE ) << "Request for sid" << rid;

    result_ptr rp = Pipeline::instance()->result( rid );
    if ( rp.isNull() )
    {
        return send404( event );
    }

    boost::function< void ( QSharedPointer< QIODevice >& ) > callback =
            boost::bind( &Api_v1::processSid, this, event, rp, _1 );
    Tomahawk::UrlHandler::getIODeviceForUrl( rp, rp->url(), callback );
}


void
Api_v1::processSid( QxtWebRequestEvent* event, Tomahawk::result_ptr& rp, QSharedPointer< QIODevice >& iodev )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( !iodev || !rp )
    {
        return send404( event ); // 503?
    }
    m_ioDevice = iodev;

    QxtWebPageEvent* e = new QxtWebPageEvent( event->sessionID, event->requestID, iodev.data() );
    e->streaming = iodev->isSequential();
    e->contentType = rp->mimetype().toLatin1();
    if ( rp->size() > 0 )
        e->headers.insert( "Content-Length", QString::number( rp->size() ) );

    postEvent( e );
}


void
Api_v1::send404( QxtWebRequestEvent* event )
{
    tDebug() << "404" << event->url.toString();
    QxtWebPageEvent* wpe = new QxtWebPageEvent( event->sessionID, event->requestID, "<h1>Not Found</h1>" );
    wpe->status = 404;
    wpe->statusMessage = "no event found";
    postEvent( wpe );
}

void
Api_v1::sendPlain404( QxtWebRequestEvent* event, const QString& message, const QString& statusmessage )
{
    QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, message.toUtf8() );
    e->contentType = "text/plain";
    e->status = 404;
    e->statusMessage = statusmessage.toLatin1().constData();
    postEvent( e );
}


void
Api_v1::stat( QxtWebRequestEvent* event )
{
    tDebug( LOGVERBOSE ) << "Got Stat request:" << event->url.toString();
    m_storedEvent = event;

    if ( !event->content.isNull() )
        tDebug( LOGVERBOSE ) << "BODY:" << event->content->readAll();

    if ( urlHasQueryItem( event->url, "auth" ) )
    {
        // check for auth status
        DatabaseCommand_ClientAuthValid* dbcmd = new DatabaseCommand_ClientAuthValid( urlQueryItemValue( event->url, "auth" ) );
        connect( dbcmd, SIGNAL( authValid( QString, QString, bool ) ), this, SLOT( statResult( QString, QString, bool ) ) );
        Database::instance()->enqueue( Tomahawk::dbcmd_ptr(dbcmd) );
    }
    else
    {
        statResult( QString(), QString(), false );
    }
}


void
Api_v1::statResult( const QString& clientToken, const QString& name, bool valid )
{
    Q_UNUSED( clientToken )
    Q_UNUSED( name )

    Q_ASSERT( m_storedEvent );
    if ( !m_storedEvent )
        return;

    QVariantMap m;
    m.insert( "name", "playdar" );
    m.insert( "version", "0.1.1" ); // TODO (needs to be >=0.1.1 for JS to work)
    m.insert( "authenticated", valid ); // TODO
    m.insert( "capabilities", QVariantList() );
    sendJSON( m, m_storedEvent );

    m_storedEvent = 0;
}


void
Api_v1::resolve( QxtWebRequestEvent* event )
{
    if ( !urlHasQueryItem( event->url, "artist" ) ||
         !urlHasQueryItem( event->url, "track" ) )
    {
        tDebug( LOGVERBOSE ) << "Malformed HTTP resolve request";
        return send404( event );
    }

    const QString artist = urlQueryItemValue( event->url, "artist" );
    const QString track = urlQueryItemValue( event->url, "track" );
    const QString album = urlQueryItemValue( event->url, "album" );

    if ( artist.trimmed().isEmpty() ||
         track.trimmed().isEmpty() )
    {
        tDebug( LOGVERBOSE ) << "Malformed HTTP resolve request";
        return send404( event );
    }

    QString qid;
    if ( urlHasQueryItem( event->url, "qid" ) )
        qid = urlQueryItemValue( event->url, "qid" );
    else
        qid = uuid();

    query_ptr qry = Query::get( artist, track, album, qid, false );
    if ( qry.isNull() )
    {
        return send404( event );
    }

    Pipeline::instance()->resolve( qry, true, true );

    QVariantMap r;
    r.insert( "qid", qid );
    sendJSON( r, event );
}


void
Api_v1::staticdata( QxtWebRequestEvent* event, const QString& file )
{
    tDebug( LOGVERBOSE ) << "STATIC request:" << event << file;

    bool whitelisted = ( file == QString( "tomahawk_auth_logo.png" ) ||
                         file.startsWith( "css/" ) ||
                         file.startsWith( "js/" ) );

    if ( whitelisted )
    {
        QFile f( RESPATH "www/" + file );
        f.open( QIODevice::ReadOnly );
        QByteArray data = f.readAll();

        QxtWebPageEvent* e = new QxtWebPageEvent( event->sessionID, event->requestID, data );

        if ( file.endsWith( ".png" ) )
            e->contentType = "image/png";
        if ( file.endsWith( ".css" ) )
            e->contentType = "text/css";
        if ( file.endsWith( ".js" ) )
            e->contentType = "application/javascript";

        postEvent( e );
    }
    else
    {
        send404( event );
        return;
    }
}


void
Api_v1::staticdata( QxtWebRequestEvent* event, const QString& path, const QString& file )
{
    return staticdata( event, path + "/" + file );
}


void
Api_v1::get_results( QxtWebRequestEvent* event )
{
    if ( !urlHasQueryItem( event->url, "qid" ) )
    {
        tDebug( LOGVERBOSE ) << "Malformed HTTP get_results request";
        send404( event );
        return;
    }

    query_ptr qry = Pipeline::instance()->query( urlQueryItemValue( event->url, "qid" ) );
    if ( qry.isNull() )
    {
        send404( event );
        return;
    }

    QVariantMap r;
    r.insert( "qid", qry->id() );
    r.insert( "poll_interval", 1300 );
    r.insert( "refresh_interval", 1000 );
    r.insert( "poll_limit", 14 );
    r.insert( "solved", qry->playable() );
    r.insert( "query", qry->toVariant() );

    QVariantList res;
    foreach( const result_ptr& rp, qry->results() )
    {
        if ( rp->isOnline() )
            res << rp->toVariant();
    }
    r.insert( "results", res );

    sendJSON( r, event );
}


void
Api_v1::sendJSON( const QVariantMap& m, QxtWebRequestEvent* event )
{
    QJson::Serializer ser;
    QByteArray ctype;
    QByteArray body = ser.serialize( m );

    if ( urlHasQueryItem( event->url, "jsonp" ) && !urlQueryItemValue( event->url, "jsonp" ).isEmpty() )
    {
        ctype = "text/javascript; charset=utf-8";
        body.prepend( QString("%1( ").arg( urlQueryItemValue( event->url, "jsonp" ) ).toLatin1() );
        body.append( " );" );
    }
    else
    {
        ctype = "appplication/json; charset=utf-8";
    }

    QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, body );
    e->contentType = ctype;
    e->headers.insert( "Content-Length", QString::number( body.length() ) );
    e->headers.insert( "Access-Control-Allow-Origin", "*" );
    postEvent( e );
    tDebug( LOGVERBOSE ) << "JSON response" << event->url.toString() << body;
}


// load an html template from a file, replace args from map
// then serve
void
Api_v1::sendWebpageWithArgs( QxtWebRequestEvent* event, const QString& filenameSource, const QHash< QString, QString >& args )
{
    if ( !QFile::exists( filenameSource ) )
        qWarning() << "Passed invalid file for html source:" << filenameSource;

    QFile f( filenameSource );
    f.open( QIODevice::ReadOnly );
    QByteArray html = f.readAll();

    foreach( const QString& param, args.keys() )
    {
        html.replace( QString( "<%%1%>" ).arg( param.toUpper() ), args.value( param ).toUtf8() );
    }
    // workaround for receiverurl
    if ( !args.keys().contains( "URL" ) )
        html.replace( QString( "<%URL%>" ).toLatin1(), QByteArray() );


    QxtWebPageEvent* e = new QxtWebPageEvent( event->sessionID, event->requestID, html );
    postEvent( e );
}


void
Api_v1::index( QxtWebRequestEvent* event )
{
    QString indexPage = RESPATH "www/index.html";
    QHash< QString, QString > args;
    sendWebpageWithArgs( event, indexPage, args );
}

void
Api_v1::apiCallFailed( QxtWebRequestEvent* event, const QString& method )
{
    sendPlain404( event, QString( "Method \"%1\" for API 1.5 not found" ).arg( method ), "Method in API 1.5 not found" );
}

void
Api_v1::sendJsonOk( QxtWebRequestEvent* event )
{
    QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, "{ \"result\": \"ok\" }" );
    e->headers.insert( "Access-Control-Allow-Origin", "*" );
    e->contentType = "application/json";
    postEvent( e );
}


void
Api_v1::sendJsonError( QxtWebRequestEvent* event, const QString& message )
{
    QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, QString( "{ \"result\": \"error\", \"error\": \"%1\" }" ).arg( message ).toUtf8().constData() );
    e->headers.insert( "Access-Control-Allow-Origin", "*" );
    e->contentType = "application/json";
    e->status = 500;
    e->statusMessage = "Method call failed.";
    postEvent( e );
}
