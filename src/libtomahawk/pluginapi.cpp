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

#include "pluginapi.h"

#include "pipeline.h"
#include "sourcelist.h"

using namespace Tomahawk;


PluginAPI::PluginAPI( Pipeline* p )
    : m_pipeline( p )
{
}


/*void
PluginAPI::reportResults( const QString& qid, const QList<QVariantMap>& vresults )
{
    QList< result_ptr > results;
    foreach( QVariantMap m, vresults )
    {
        result_ptr rp( new Result( m ) );
        results.append( rp );
    }
    m_pipeline->reportResults( QID( qid ), results );
}*/


void
PluginAPI::addSource( source_ptr s )
{
//    SourceList::instance()->add( s );
}


void
PluginAPI::removeSource( source_ptr s )
{
//    SourceList::instance()->remove( s );
}


void
PluginAPI::addResolver( Resolver* r )
{
    Pipeline::instance()->addResolver( r );
}
