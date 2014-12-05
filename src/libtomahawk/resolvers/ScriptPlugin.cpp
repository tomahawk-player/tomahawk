/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
 *   Copyright (C) 2014  Dominik Schmidt <domme@tomahawk-player.org>
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
#include "ScriptPlugin.h"

#include "ScriptObject.h"
#include "../utils/Logger.h"


// TODO: register factory methods instead of hardcoding all plugin types in here
#include "../utils/LinkGenerator.h"
#include "ScriptLinkGeneratorPlugin.h"

using namespace Tomahawk;

static QString
requestIdGenerator()
{
    static int requestCounter = 0;
    return QString::number( ++requestCounter );

}


ScriptJob*
ScriptPlugin::invoke( ScriptObject* scriptObject, const QString& methodName, const QVariantMap& arguments )
{
    QString requestId = requestIdGenerator();

    ScriptJob* job = new ScriptJob( requestId, scriptObject, methodName, arguments );
    connect( job, SIGNAL( destroyed( QString ) ), SLOT( onJobDeleted( QString ) ) );
    m_jobs.insert( requestId, job );

    return job;
}


void
ScriptPlugin::reportScriptJobResult( const QVariantMap& result )
{
    tLog() << Q_FUNC_INFO << result;
    const QString requestId = result[ "requestId" ].toString();
    Q_ASSERT( !requestId.isEmpty() );

    ScriptJob* job = m_jobs.value( requestId );
    Q_ASSERT( job );

    // got a successful job result
    if ( result[ "error"].isNull() )
    {
        const QVariantMap data = result[ "data" ].toMap();

        job->reportResults( data );
    }
    else
    {
        job->reportFailure( result[ "error" ].toString() );
    }
}


void
ScriptPlugin::registerScriptPlugin( const QString& type, const QString& objectId )
{
    ScriptObject* object = m_objects.value( objectId );
    if( !object )
    {
        object = new ScriptObject( objectId, this );
        m_objects.insert( objectId, object );
    }

    if ( type == "linkGenerator" )
    {
        tLog() << "Got link generator plugin";
        ScriptLinkGeneratorPlugin* lgp = new ScriptLinkGeneratorPlugin( object );
        Utils::LinkGenerator::instance()->addPlugin( lgp );
    }
    else
    {
        tLog() << "This plugin type is not handled by Tomahawk";
        Q_ASSERT( false );
    }
}


void
ScriptPlugin::onJobDeleted( const QString& jobId )
{
    m_jobs.remove( jobId );
}
