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
#include "ScriptAccount.h"

#include "ScriptObject.h"
#include "../utils/Logger.h"
#include "../Typedefs.h"

// TODO: register factory methods instead of hardcoding all plugin types in here
#include "../utils/LinkGenerator.h"
#include "ScriptLinkGeneratorPlugin.h"
#include "ScriptInfoPlugin.h"

using namespace Tomahawk;


ScriptAccount::ScriptAccount( const QString& name )
    : QObject()
    , m_name( name )
{
}


static QString
requestIdGenerator()
{
    static int requestCounter = 0;
    return QString::number( ++requestCounter );

}


ScriptJob*
ScriptAccount::invoke( const scriptobject_ptr& scriptObject, const QString& methodName, const QVariantMap& arguments )
{
    QString requestId = requestIdGenerator();

    ScriptJob* job = new ScriptJob( requestId, scriptObject, methodName, arguments );
    // TODO: setParent through QueuedConnection


    connect( job, SIGNAL( destroyed( QString ) ), SLOT( onJobDeleted( QString ) ) );
    m_jobs.insert( requestId, job );

    return job;
}


void
ScriptAccount::reportScriptJobResult( const QVariantMap& result )
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
ScriptAccount::registerScriptPlugin( const QString& type, const QString& objectId )
{
    scriptobject_ptr object = m_objects.value( objectId );
    if( !object )
    {
        object = scriptobject_ptr( new ScriptObject( objectId, this ), &ScriptObject::deleteLater );
        object->setWeakRef( object.toWeakRef() );
        connect( object.data(), SIGNAL( destroyed( QObject* ) ), SLOT( onScriptObjectDeleted() ) );
        m_objects.insert( objectId, object );
    }

    scriptPluginFactory( type, object );
}


void
ScriptAccount::onScriptObjectDeleted()
{
    foreach( const scriptobject_ptr& object, m_objects.values() )
    {
        if ( object.isNull() )
        {
            m_objects.remove( m_objects.key( object ) );
            break;
        }
    }
}


void
ScriptAccount::scriptPluginFactory( const QString& type, const scriptobject_ptr& object )
{
    if ( type == "linkGenerator" )
    {
        tLog() << "Got link generator plugin";
        ScriptLinkGeneratorPlugin* lgp = new ScriptLinkGeneratorPlugin( object );
        Utils::LinkGenerator::instance()->addPlugin( lgp );
    }
    else if ( type == "infoPlugin" )
    {
        // create infoplugin instance
        ScriptInfoPlugin* scriptInfoPlugin = new ScriptInfoPlugin( object, m_name );
        Tomahawk::InfoSystem::InfoPluginPtr infoPlugin( scriptInfoPlugin );

        // move it to infosystem thread
        infoPlugin->moveToThread( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() );

        // add it to infosystem
        Tomahawk::InfoSystem::InfoSystem::instance()->addInfoPlugin( infoPlugin );
    }
    else
    {
        tLog() << "This plugin type is not handled by Tomahawk";
        Q_ASSERT( false );
    }
}


void
ScriptAccount::onJobDeleted( const QString& jobId )
{
    m_jobs.remove( jobId );
}
