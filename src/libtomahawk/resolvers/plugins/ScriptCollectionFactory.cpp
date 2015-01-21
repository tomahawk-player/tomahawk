/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2015  Dominik Schmidt <domme@tomahawk-player.org>
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
#include "ScriptCollectionFactory.h"

#include "SourceList.h"
#include "../ScriptAccount.h"

using namespace Tomahawk;

void ScriptCollectionFactory::addPlugin( const QSharedPointer<ScriptCollection>& collection ) const
{
    // FIXME: no need for the same javascript call, already done in createPlugin
    collection->parseMetaData();

    collection->setOnline( true );
    SourceList::instance()->addScriptCollection( collection );
}

void ScriptCollectionFactory::removePlugin( const QSharedPointer<ScriptCollection>& collection ) const
{
    collection->setOnline( false );
    SourceList::instance()->removeScriptCollection( collection );
}

QSharedPointer< ScriptCollection > ScriptCollectionFactory::createPlugin( const scriptobject_ptr& object, ScriptAccount* scriptAccount )
{
    const QVariantMap collectionInfo =  object->syncInvoke( "collection" ).toMap();

    if ( collectionInfo.isEmpty() ||
            !collectionInfo.contains( "prettyname" ) ||
            !collectionInfo.contains( "description" ) )
        return QSharedPointer< ScriptCollection >();

    // at this point we assume that all the tracks browsable through a resolver belong to the local source
    Tomahawk::ScriptCollection* sc = new Tomahawk::ScriptCollection( object, SourceList::instance()->getLocal(), scriptAccount );
    QSharedPointer<ScriptCollection> collection( sc );
    collection->setWeakRef( collection.toWeakRef() );

    collection->parseMetaData( collectionInfo );

    return collection;
}
