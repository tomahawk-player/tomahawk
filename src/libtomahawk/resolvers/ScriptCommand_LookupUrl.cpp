/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "ScriptCommand_LookupUrl_p.h"

#include "PlaylistEntry.h"

ScriptCommand_LookupUrl::ScriptCommand_LookupUrl( Tomahawk::ExternalResolver* resolver, const QString& url, QObject* parent )
    : ScriptCommand( parent )
    , d_ptr( new ScriptCommand_LookupUrlPrivate( this, resolver, url ) )
{
}

ScriptCommand_LookupUrl::~ScriptCommand_LookupUrl()
{
    delete d_ptr;
}


void
ScriptCommand_LookupUrl::enqueue()
{
    Q_D( ScriptCommand_LookupUrl );
    d->resolver->enqueue( QSharedPointer< ScriptCommand >( this ) );
}


void
ScriptCommand_LookupUrl::exec()
{
    Q_D( ScriptCommand_LookupUrl );
    connect( d->resolver, SIGNAL( informationFound( QString , QSharedPointer<QObject> ) ),
             this, SLOT( onResolverDone( QString, QSharedPointer<QObject> ) ) );

    d->resolver->lookupUrl( d->url );
}


void
ScriptCommand_LookupUrl::reportFailure()
{
    Q_D( ScriptCommand_LookupUrl );
    emit information( d->url, QSharedPointer<QObject>() );
    emit done();
}


void
ScriptCommand_LookupUrl::onResolverDone( const QString& url, const QSharedPointer<QObject>& _information )
{
    Q_D( ScriptCommand_LookupUrl );

    qDebug() << Q_FUNC_INFO << url << _information.isNull();
    if ( url != d->url )
    {
        // This data is not for us, skip.
        return;
    }
    emit information( d->url, _information );
    emit done();
}
