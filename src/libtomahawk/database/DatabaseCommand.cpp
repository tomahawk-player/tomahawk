/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "DatabaseCommand_p.h"

#include "utils/Logger.h"

#include "Source.h"


namespace Tomahawk
{

DatabaseCommand::DatabaseCommand( QObject* parent )
    : QObject( parent )
    , d_ptr( new DatabaseCommandPrivate( this ) )
{
}


DatabaseCommand::DatabaseCommand( const Tomahawk::source_ptr& src, QObject* parent )
    : QObject( parent )
    , d_ptr( new DatabaseCommandPrivate( this, src ) )
{
}


DatabaseCommand::DatabaseCommand( const DatabaseCommand& other )
    : QObject( other.parent() )
    , d_ptr( new DatabaseCommandPrivate( this ) )
{
}


DatabaseCommand::~DatabaseCommand()
{
}


DatabaseCommand::State
DatabaseCommand::state() const
{
    Q_D( const DatabaseCommand );
    return d->state;
}


void
DatabaseCommand::_exec( DatabaseImpl* lib )
{
    Q_D( DatabaseCommand );
    d->state = RUNNING;
    emitRunning();
    exec( lib );
    d->state = FINISHED;
}


void
DatabaseCommand::setSource( const Tomahawk::source_ptr& s )
{
    Q_D( DatabaseCommand );
    d->source = s;
}


const Tomahawk::source_ptr&
DatabaseCommand::source() const
{
    Q_D( const DatabaseCommand );
    return d->source;
}


QVariant
DatabaseCommand::data() const
{
    Q_D( const DatabaseCommand );
    return d->data;
}


void
DatabaseCommand::setData( const QVariant& data )
{
    Q_D( DatabaseCommand );
    d->data = data;
}


QString
DatabaseCommand::guid() const
{
    Q_D( const DatabaseCommand );
    if( d->guid.isEmpty() )
        d->guid = uuid();

    return d->guid;
}


void
DatabaseCommand::setGuid( const QString& g)
{
    Q_D( DatabaseCommand );
    d->guid = g;
}


void
DatabaseCommand::emitFinished()
{
    Q_D( DatabaseCommand );
    emit finished( d->ownRef.toStrongRef() );
    emit finished();
}

void
DatabaseCommand::emitCommitted()
{
    Q_D( DatabaseCommand );
    emit committed( d->ownRef.toStrongRef() );
    emit committed();
}


void
DatabaseCommand::emitRunning()
{
    Q_D( DatabaseCommand );
    emit running( d->ownRef.toStrongRef() );
    emit running();
}


QWeakPointer< DatabaseCommand >
DatabaseCommand::weakRef() const
{
    Q_D( const DatabaseCommand );
    return d->ownRef;
}


void
DatabaseCommand::setWeakRef( QWeakPointer< DatabaseCommand > weakRef )
{
    Q_D( DatabaseCommand );
    d->ownRef = weakRef;
}

DatabaseCommand::DatabaseCommand( QObject* parent, DatabaseCommandPrivate* d)
    : QObject( parent )
    , d_ptr( d )
{

}

}
