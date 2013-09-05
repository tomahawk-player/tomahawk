/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
 *   Copyright (C) 2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright (C) 2011-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright (C) 2013, Uwe L. Korn <uwelk@xhochy.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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

#include "ShortLinkHelper_p.h"

#include "utils/Closure.h"
#include "utils/NetworkAccessManager.h"
#include "utils/TomahawkUtils.h"
#include "Playlist.h"
#include "Source.h"
#include "Track.h"

#include <qjson/serializer.h>

namespace Tomahawk {
namespace Utils {

ShortLinkHelper::ShortLinkHelper(QObject *parent)
    : QObject( parent )
    , d_ptr( new ShortLinkHelperPrivate( this ) )
{
}

ShortLinkHelper::~ShortLinkHelper()
{
}


void
ShortLinkHelper::shortLink( const Tomahawk::playlist_ptr& pl )
{
    Q_D( ShortLinkHelper );
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "shortLink", Qt::QueuedConnection,
                                   Q_ARG( const Tomahawk::playlist_ptr&, pl) );
        return;
    }

    if ( !pl->loaded() )
    {
        pl->loadRevision();
    }
    if ( pl->busy() || !pl->loaded() )
    {
        NewClosure( pl.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ),
                    this, SLOT( shortLink( Tomahawk::playlist_ptr ) ), pl );
        return;
    }

    QVariantMap m;
    m[ "title" ] = pl->title();
    m[ "creator" ] = pl->author().isNull() ? "" : pl->author()->friendlyName();
    QVariantList tracks;
    foreach( const plentry_ptr& pl, pl->entries() )
    {
        if ( pl->query().isNull() )
            continue;

        QVariantMap track;
        track[ "title" ] = pl->query()->track()->track();
        track[ "creator" ] = pl->query()->track()->artist();
        track[ "album" ] = pl->query()->track()->album();

        tracks << track;
    }
    m[ "track" ] = tracks;

    QVariantMap jspf;
    jspf["playlist"] = m;

    QJson::Serializer s;
    QByteArray msg = s.serialize( jspf );

    // No built-in Qt facilities for doing a FORM POST. So we build the payload ourselves...
    const QByteArray boundary = "----------------------------2434992cccab";
    QByteArray data( QByteArray( "--" + boundary + "\r\n" ) );
    data += "Content-Disposition: form-data; name=\"data\"; filename=\"playlist.jspf\"\r\n";
    data += "Content-Type: application/octet-stream\r\n\r\n";
    data += msg;
    data += "\r\n\r\n";
    data += "--" + boundary + "--\r\n\r\n";

    const QUrl url( QString( "%1/p/").arg( hostname() ) );
    QNetworkRequest req( url );
    req.setHeader( QNetworkRequest::ContentTypeHeader, QString( "multipart/form-data; boundary=%1" ).arg( QString::fromLatin1( boundary ) ) );
    d->reply = Tomahawk::Utils::nam()->post( req, data );

    NewClosure( d->reply, SIGNAL( finished() ),
                this, SLOT( shortLinkRequestFinished( Tomahawk::playlist_ptr ) ), pl );
    connect( d->reply, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( shortenLinkRequestError( QNetworkReply::NetworkError ) ) );
}


void
ShortLinkHelper::shortenLink( const QUrl& url, const QVariant& callbackObj )
{
    Q_D( ShortLinkHelper );
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "shortenLink", Qt::QueuedConnection,
                                   Q_ARG( const QUrl&, url ),
                                   Q_ARG( const QVariant&, callbackObj ) );
        return;
    }

    QNetworkRequest request;
    request.setUrl( url );

    d->reply = Tomahawk::Utils::nam()->get( request );
    if ( callbackObj.isValid() )
        d->reply->setProperty( "callbackobj", callbackObj );
    connect( d->reply, SIGNAL( finished() ), SLOT( shortenLinkRequestFinished() ) );
    connect( d->reply, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( shortenLinkRequestError( QNetworkReply::NetworkError ) ) );
}


QString
ShortLinkHelper::hostname() const
{
    return QString( "http://toma.hk" );
}


void
ShortLinkHelper::shortLinkRequestFinished( const playlist_ptr& playlist )
{
    Q_D( ShortLinkHelper );
    Q_ASSERT( d->reply );

    const QByteArray raw = d->reply->readAll();
    const QUrl url = QUrl::fromUserInput( raw );
    const QByteArray data = TomahawkUtils::percentEncode( url );

    emit shortLinkReady( playlist, QString( data.constData() ) );
    emit done();

    d->reply->deleteLater();
}

void
ShortLinkHelper::shortenLinkRequestFinished()
{
    Q_D( ShortLinkHelper );
    bool error = false;

    // NOTE: this should never happen
    if ( !d->reply )
    {
        emit shortLinkReady( QUrl( "" ), QUrl( "" ), QVariantMap() );
        emit done();
        return;
    }

    QVariant callbackObj;
    if ( d->reply->property( "callbackobj" ).isValid() )
        callbackObj = d->reply->property( "callbackobj" );

    // Check for the redirect attribute, as this should be the shortened link
    QVariant urlVariant = d->reply->attribute( QNetworkRequest::RedirectionTargetAttribute );

    // NOTE: this should never happen
    if ( urlVariant.isNull() || !urlVariant.isValid() )
        error = true;

    QUrl longUrl = d->reply->request().url();
    QUrl shortUrl = urlVariant.toUrl();

    // NOTE: this should never happen
    if ( !shortUrl.isValid() )
        error = true;

    if ( !error )
        emit shortLinkReady( longUrl, shortUrl, callbackObj );
    else
        emit shortLinkReady( longUrl, longUrl, callbackObj );
    emit done();

    d->reply->deleteLater();
}


void
ShortLinkHelper::shortenLinkRequestError( QNetworkReply::NetworkError )
{
    Q_D( ShortLinkHelper );

    // NOTE: this should never happen
    if ( !d->reply )
    {
        emit shortLinkReady( QUrl( "" ), QUrl( "" ), QVariantMap() );
        emit done();
        return;
    }

    QVariantMap callbackMap;
    if ( d->reply->property( "callbackMap" ).canConvert< QVariantMap >() && !d->reply->property( "callbackMap" ).toMap().isEmpty() )
        callbackMap = d->reply->property( "callbackMap" ).toMap();
    d->reply->deleteLater();
    emit shortLinkReady( QUrl( "" ), QUrl( "" ), callbackMap );
    emit done();
}

} // namespace Utils
} // namespace Tomahawk
