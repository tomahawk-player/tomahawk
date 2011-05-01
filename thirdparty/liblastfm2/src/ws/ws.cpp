/*
   Copyright 2009 Last.fm Ltd. 
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ws.h"
#include "../core/misc.h"
#include "NetworkAccessManager.h"
#include <QCoreApplication>
#include <QDomDocument>
#include <QDomElement>
#include <QLocale>
#include <QStringList>
#include <QThread>
#include <QUrl>

static QMap< QThread*, QNetworkAccessManager* > threadNamMap;

QString 
lastfm::ws::host()
{
    QStringList const args = QCoreApplication::arguments();
    if (args.contains( "--debug"))
        return "ws.staging.audioscrobbler.com";

    int const n = args.indexOf( "--host" );
    if (n != -1 && args.count() > n+1)
        return args[n+1];

    return LASTFM_WS_HOSTNAME;
}

static QUrl url()
{
    QUrl url;
    url.setScheme( "http" );
    url.setHost( lastfm::ws::host() );
    url.setPath( "/2.0/" );
    return url;
}

static QString iso639()
{ 
    return QLocale().name().left( 2 ).toLower(); 
}

void autograph( QMap<QString, QString>& params )
{
    params["api_key"] = lastfm::ws::ApiKey;
    params["lang"] = iso639();
}

static void sign( QMap<QString, QString>& params, bool sk = true )
{
    autograph( params );
    // it's allowed for sk to be null if we this is an auth call for instance
    if (sk && lastfm::ws::SessionKey.size())
        params["sk"] = lastfm::ws::SessionKey;

    QString s;
    QMapIterator<QString, QString> i( params );
    while (i.hasNext()) {
        i.next();
        s += i.key() + i.value();
    }
    s += lastfm::ws::SharedSecret;

    params["api_sig"] = lastfm::md5( s.toUtf8() );
}


QNetworkReply*
lastfm::ws::get( QMap<QString, QString> params )
{
    sign( params );
    QUrl url = ::url();
    // Qt setQueryItems doesn't encode a bunch of stuff, so we do it manually
    QMapIterator<QString, QString> i( params );
    while (i.hasNext()) {
        i.next();
        QByteArray const key = QUrl::toPercentEncoding( i.key() );
        QByteArray const value = QUrl::toPercentEncoding( i.value() );
        url.addEncodedQueryItem( key, value );
    }
    
    qDebug() << url;
    
    return nam()->get( QNetworkRequest(url) );
}


QNetworkReply*
lastfm::ws::post( QMap<QString, QString> params, bool sk )
{   
    sign( params, sk ); 
    QByteArray query;
    QMapIterator<QString, QString> i( params );
    while (i.hasNext()) {
        i.next();
        query += QUrl::toPercentEncoding( i.key() )
               + '='
               + QUrl::toPercentEncoding( i.value() )
               + '&';
    }

    return nam()->post( QNetworkRequest(url()), query );
}


QByteArray
lastfm::ws::parse( QNetworkReply* reply ) throw( ParseError )
{
    try
    {
        QByteArray data = reply->readAll();

        if (!data.size())
            throw MalformedResponse;
            
        QDomDocument xml;
        xml.setContent( data );
        QDomElement lfm = xml.documentElement();

        if (lfm.isNull())
            throw MalformedResponse;

        QString const status = lfm.attribute( "status" );
        QDomElement error = lfm.firstChildElement( "error" );
        uint const n = lfm.childNodes().count();

        // no elements beyond the lfm is perfectably acceptable <-- wtf?
        // if (n == 0) // nothing useful in the response
        if (status == "failed" || (n == 1 && !error.isNull()) )
            throw error.isNull()
                    ? MalformedResponse
                    : Error( error.attribute( "code" ).toUInt() );

        switch (reply->error())
        {
            case QNetworkReply::RemoteHostClosedError:
            case QNetworkReply::ConnectionRefusedError:
            case QNetworkReply::TimeoutError:
            case QNetworkReply::ContentAccessDenied:
            case QNetworkReply::ContentOperationNotPermittedError:
            case QNetworkReply::UnknownContentError:
            case QNetworkReply::ProtocolInvalidOperationError:
            case QNetworkReply::ProtocolFailure:
                throw TryAgainLater;

            case QNetworkReply::NoError:
            default:
                break;
        }

        //FIXME pretty wasteful to parse XML document twice..
        return data;
    }
    catch (Error e)
    {
        switch (e)
        {
            case OperationFailed:
            case InvalidApiKey:
            case InvalidSessionKey:
                // NOTE will never be received during the LoginDialog stage
                // since that happens before this slot is registered with
                // QMetaObject in App::App(). Neat :)
                QMetaObject::invokeMethod( qApp, "onWsError", Q_ARG( lastfm::ws::Error, e ) );
            default:
                throw ParseError(e);
        }
    }

    // bit dodgy, but prolly for the best
    reply->deleteLater();
}


QNetworkAccessManager*
lastfm::nam()
{
    QThread* thread = QThread::currentThread();
    if ( !threadNamMap.contains( thread ) )
    {
        NetworkAccessManager* newNam = new NetworkAccessManager();
        threadNamMap[thread] = newNam;
        return newNam;
    }
    
    return threadNamMap[thread];
}


void
lastfm::setNetworkAccessManager( QNetworkAccessManager* nam )
{
    QThread* thread = QThread::currentThread();
    if ( threadNamMap.contains( thread ) )
    {
        delete threadNamMap[thread];
        threadNamMap[thread] = 0;
    }
    
    threadNamMap[thread] = nam;
}


/** This useful function, fromHttpDate, comes from QNetworkHeadersPrivate
  * in qnetworkrequest.cpp.  Qt copyright and license apply. */
static QDateTime QByteArrayToHttpDate(const QByteArray &value)
{
    // HTTP dates have three possible formats:
    //  RFC 1123/822      -   ddd, dd MMM yyyy hh:mm:ss "GMT"
    //  RFC 850           -   dddd, dd-MMM-yy hh:mm:ss "GMT"
    //  ANSI C's asctime  -   ddd MMM d hh:mm:ss yyyy
    // We only handle them exactly. If they deviate, we bail out.

    int pos = value.indexOf(',');
    QDateTime dt;
    if (pos == -1) {
        // no comma -> asctime(3) format
        dt = QDateTime::fromString(QString::fromLatin1(value), Qt::TextDate);
    } else {
        // eat the weekday, the comma and the space following it
        QString sansWeekday = QString::fromLatin1(value.constData() + pos + 2);

        QLocale c = QLocale::c();
        if (pos == 3)
            // must be RFC 1123 date
            dt = c.toDateTime(sansWeekday, QLatin1String("dd MMM yyyy hh:mm:ss 'GMT"));
        else
            // must be RFC 850 date
            dt = c.toDateTime(sansWeekday, QLatin1String("dd-MMM-yy hh:mm:ss 'GMT'"));
    }

    if (dt.isValid())
        dt.setTimeSpec(Qt::UTC);
    return dt;
}


QDateTime
lastfm::ws::expires( QNetworkReply* reply )
{
    return QByteArrayToHttpDate( reply->rawHeader( "Expires" ) );
}


namespace lastfm
{
    namespace ws
    {
        QString SessionKey;
        QString Username;

        /** we leave these unset as you can't use the webservices without them
          * so lets make the programmer aware of it during testing by crashing */
        const char* SharedSecret;
        const char* ApiKey;

        /** if this is found set to "" we conjure ourselves a suitable one */
        const char* UserAgent = 0;   
    }
}


QDebug operator<<( QDebug, lastfm::ws::Error );
