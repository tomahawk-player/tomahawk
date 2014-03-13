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

#ifndef TOMAHAWK_URLHANDLER_H
#define TOMAHAWK_URLHANDLER_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <boost/function.hpp>

typedef boost::function< void( const Tomahawk::result_ptr&, const QString&,
                               boost::function< void( QSharedPointer< QIODevice >& ) > )> IODeviceFactoryFunc;
typedef boost::function< void( const Tomahawk::result_ptr&, const QString&,
                               boost::function< void( const QString& ) > )> UrlTranslatorFunc;
typedef boost::function< void ( QSharedPointer< QIODevice >& ) > IODeviceCallback;


namespace Tomahawk
{

namespace UrlHandler
{

    DLLEXPORT void getIODeviceForUrl( const Tomahawk::result_ptr&, const QString& url, boost::function< void ( QSharedPointer< QIODevice >& ) > callback );
    DLLEXPORT void registerIODeviceFactory( const QString& proto, IODeviceFactoryFunc fac );
    DLLEXPORT void localFileIODeviceFactory( const Tomahawk::result_ptr& result, const QString& url,
                                             boost::function< void ( QSharedPointer< QIODevice >& ) > callback );
    DLLEXPORT void httpIODeviceFactory( const Tomahawk::result_ptr& result, const QString& url,
                                       boost::function< void ( QSharedPointer< QIODevice >& ) > callback );

    DLLEXPORT void getUrlTranslation( const Tomahawk::result_ptr& result, const QString& url,
                                      boost::function< void ( const QString& ) > callback );
    DLLEXPORT void registerUrlTranslator( const QString &proto, UrlTranslatorFunc fac );

} // namespace UrlHandler

} // namespace Tomahawk

#endif // TOMAHAWK_URLHANDLER_H
