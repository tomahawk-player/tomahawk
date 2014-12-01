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

#include "ScriptJob.h"
#include "ScriptObject.h"
#include <QMetaObject>

using namespace Tomahawk;

ScriptJob::ScriptJob( const QString& id, ScriptObject* scriptObject, const QString& methodName, const QVariantMap& arguments )
    : QObject( scriptObject )
    , m_id( id )
    , m_scriptObject( scriptObject )
    , m_methodName( methodName )
    , m_arguments( arguments )
{
}


ScriptJob::~ScriptJob()
{
    //FIXME: probably not necessary if we change the inheritance order
    if ( !m_id.isEmpty() )
    {
        Q_ASSERT( m_scriptObject );
        m_scriptObject->removeJob( this );
    }
}

void
ScriptJob::start()
{
    m_scriptObject->startJob( this );
}


ScriptObject*
ScriptJob::scriptObject() const
{
    return m_scriptObject;
}


const QString
ScriptJob::id() const
{
    return m_id;
}


const QString
ScriptJob::methodName() const
{
    return m_methodName;
}


const QVariantMap
ScriptJob::arguments() const
{
    return m_arguments;
}


void
ScriptJob::reportResults( const QVariantMap& data )
{
    m_data = data;
    emit done( data );
}


void
ScriptJob::reportFailure()
{

}

