/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013-2014, Uwe L. Korn <uwelk@xhochy.com>
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

#include "UrlHandler_p.h"

#include "utils/NetworkAccessManager.h"
#include "Result.h"

#include <QFile>

#include <boost/bind.hpp>

Q_DECLARE_METATYPE( IODeviceCallback )

namespace Tomahawk {
namespace UrlHandler {

QMap< QString, IODeviceFactoryFunc > iofactories;
QMap< QString, UrlTranslatorFunc > urltranslators;

void
initialiseDefaultIOFactories()
{
    {
        // _1 = result, _2 = callback function for IODevice
        IODeviceFactoryFunc fac = boost::bind( localFileIODeviceFactory, _1, _2, _3 );
        iofactories.insert( "file", fac );
    }

    {
        IODeviceFactoryFunc fac = boost::bind( httpIODeviceFactory, _1, _2, _3 );
        iofactories.insert( "http", fac );
        iofactories.insert( "https", fac );
    }

}

void
registerIODeviceFactory( const QString &proto, IODeviceFactoryFunc fac )
{
    if ( iofactories.isEmpty() )
    {
        initialiseDefaultIOFactories();
    }

    iofactories.insert( proto, fac );
}


void
getIODeviceForUrl( const Tomahawk::result_ptr& result, const QString& url,
                            boost::function< void ( QSharedPointer< QIODevice >& ) > callback )
{
    if ( iofactories.isEmpty() )
    {
        initialiseDefaultIOFactories();
    }

    QSharedPointer<QIODevice> sp;

    QRegExp rx( "^([a-zA-Z0-9]+)://(.+)$" );
    if ( rx.indexIn( url ) == -1 )
    {
        callback( sp );
        return;
    }

    const QString proto = rx.cap( 1 );
    if ( !iofactories.contains( proto ) )
    {
        callback( sp );
        return;
    }

    // JSResolverHelper::customIODeviceFactory is async!
    iofactories.value( proto )( result, url, callback );
}


void
localFileIODeviceFactory( const Tomahawk::result_ptr&, const QString& url,
                                   boost::function< void ( QSharedPointer< QIODevice >& ) > callback )
{
    // ignore "file://" at front of url
    QFile* io = new QFile( url.mid( QString( "file://" ).length() ) );
    if ( io )
        io->open( QIODevice::ReadOnly );

    //boost::functions cannot accept temporaries as parameters
    QSharedPointer< QIODevice > sp = QSharedPointer<QIODevice>( io );
    callback( sp );
}


void
httpIODeviceFactory( const Tomahawk::result_ptr&, const QString& url,
                              boost::function< void ( QSharedPointer< QIODevice >& ) > callback )
{
    QNetworkRequest req( url );
    // Follow HTTP Redirects
    NetworkReply* reply = new NetworkReply( Tomahawk::Utils::nam()->get( req ) );
    qRegisterMetaType<NetworkReply*>("NetworkReply*");
    qRegisterMetaType<IODeviceCallback>("IODeviceCallback");
    HttpIODeviceReadyHandler* handler = new HttpIODeviceReadyHandler( reply, callback );
    reply->connect( reply, SIGNAL( finalUrlReached() ),
                    handler, SLOT( called() ));
}


void
getUrlTranslation( const Tomahawk::result_ptr& result, const QString& url, boost::function< void ( const QString& ) > callback )
{
    QRegExp rx( "^([a-zA-Z0-9]+)://(.+)$" );
    if ( rx.indexIn( url ) == -1 )
    {
        callback( QString() );
        return;
    }

    const QString proto = rx.cap( 1 );
    if ( !urltranslators.contains( proto ) )
    {
        callback( url );
        return;
    }

    urltranslators.value( proto )( result, url, callback );
}

void
registerUrlTranslator( const QString &proto, UrlTranslatorFunc fac )
{
    urltranslators.insert( proto, fac );
}


} // namespace UrlHandler
} // namespace Tomahawk
