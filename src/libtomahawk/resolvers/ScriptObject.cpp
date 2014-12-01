/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,    Dominik Schmidt <domme@tomahawk-player.org>
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
#include "ScriptObject_p.h"

#include "ScriptPlugin.h"


using namespace Tomahawk;

ScriptObject::ScriptObject( const QString& id, ScriptPlugin* parent )
    : QObject( parent )
    , d_ptr( new ScriptObjectPrivate( this, id, parent ))
{
}


ScriptObject::~ScriptObject()
{
}


ScriptJob*
ScriptObject::invoke( const QString& methodName, const QVariantMap& arguments )
{
    Q_D( ScriptObject );

    return d->scriptPlugin->invoke( this, methodName, arguments );
}


QString
ScriptObject::id() const
{
    Q_D( const ScriptObject );

    return d->id;
}


void
ScriptObject::startJob( ScriptJob* scriptJob )
{
    Q_D( const ScriptObject );

    d->scriptPlugin->startJob( scriptJob );
}


void
ScriptObject::removeJob( ScriptJob* scriptJob )
{
    Q_D( const ScriptObject );

    Q_ASSERT( d->scriptPlugin );
    d->scriptPlugin->removeJob( scriptJob );
}

