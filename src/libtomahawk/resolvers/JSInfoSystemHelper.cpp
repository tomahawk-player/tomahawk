/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
*
*   Copyright 2014, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "JSInfoSystemHelper_p.h"
#include "JSInfoPlugin.h"

#include "JSPlugin.h"
#include "../utils/Logger.h"

using namespace Tomahawk;

JSInfoSystemHelper::JSInfoSystemHelper( JSPlugin* parent )
    : QObject( parent )
    , d_ptr( new JSInfoSystemHelperPrivate( this, parent ) )
{
}


JSInfoSystemHelper::~JSInfoSystemHelper()
{
}


QStringList JSInfoSystemHelper::requiredScriptPaths() const
{
    return QStringList()
        << RESPATH "js/tomahawk-infosystem.js";
}


void
JSInfoSystemHelper::nativeAddInfoPlugin( int id )
{
    Q_D( JSInfoSystemHelper );

    // create infoplugin instance
    JSInfoPlugin* jsInfoPlugin = new JSInfoPlugin( id, d->scriptPlugin );
    Tomahawk::InfoSystem::InfoPluginPtr infoPlugin( jsInfoPlugin );

    // move it to infosystem thread
    infoPlugin->moveToThread( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() );

    // add it to local list of infoplugins
    d->infoPlugins[id] = jsInfoPlugin;

    // add it to infosystem
    Tomahawk::InfoSystem::InfoSystem::instance()->addInfoPlugin( infoPlugin );
}


void
JSInfoSystemHelper::nativeRemoveInfoPlugin( int id )
{
    Q_UNUSED( id );
    tLog() << "Removing Info plugins from JS is not implemented yet";
    Q_ASSERT( false );
}


void
JSInfoSystemHelper::nativeAddInfoRequestResult( int infoPluginId, int requestId, int maxAge, const QVariantMap& returnedData )
{
    Q_D( JSInfoSystemHelper );

    if ( !d->infoPlugins[ infoPluginId ] )
    {
        Q_ASSERT( d->infoPlugins[ infoPluginId ] );
        return;
    }

    d->infoPlugins[ infoPluginId ]->addInfoRequestResult( requestId, maxAge, returnedData );
}


void
JSInfoSystemHelper::nativeGetCachedInfo( int infoPluginId, int requestId, int newMaxAge, const QVariantMap& criteria )
{
    Q_D( JSInfoSystemHelper );

    if ( !d->infoPlugins[ infoPluginId ] )
    {
        Q_ASSERT( d->infoPlugins[ infoPluginId ] );
        return;
    }

    d->infoPlugins[ infoPluginId ]->emitGetCachedInfo( requestId, criteria, newMaxAge );
}


void JSInfoSystemHelper::nativeDataError( int infoPluginId, int requestId )
{
    Q_D( JSInfoSystemHelper );

    if ( !d->infoPlugins[ infoPluginId ] )
    {
        Q_ASSERT( d->infoPlugins[ infoPluginId ] );
        return;
    }

    d->infoPlugins[ infoPluginId ]->emitInfo( requestId, QVariantMap() );
}
