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
#include <QThread>

using namespace Tomahawk;

ScriptJob::ScriptJob( const QString& id, const scriptobject_ptr& scriptObject, const QString& methodName, const QVariantMap& arguments )
    : m_error( false )
    , m_id( id )
    , m_scriptObject( scriptObject )
    , m_methodName( methodName )
    , m_arguments( arguments )
{
}


ScriptJob::~ScriptJob()
{
    emit destroyed( m_id );
}

void
ScriptJob::start()
{
    m_scriptObject->startJob( this );
}


bool
ScriptJob::error() const
{
    return m_error;
}


scriptobject_ptr
ScriptJob::scriptObject() const
{
    return m_scriptObject;
}


QString
ScriptJob::id() const
{
    return m_id;
}


QString
ScriptJob::methodName() const
{
    return m_methodName;
}


QVariantMap
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
ScriptJob::reportFailure( const QString& errorMessage )
{
    emit error( errorMessage );

    reportResults( QVariantMap() );
}
