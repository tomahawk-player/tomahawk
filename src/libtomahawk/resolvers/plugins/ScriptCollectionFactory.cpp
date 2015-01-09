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

#include <QFileInfo>

using namespace Tomahawk;

void ScriptCollectionFactory::addPlugin( const QSharedPointer<ScriptCollection>& collection ) const
{
    SourceList::instance()->addScriptCollection( collection );
}

void ScriptCollectionFactory::removePlugin( const QSharedPointer<ScriptCollection>& collection ) const
{
    SourceList::instance()->removeScriptCollection( collection );
}

const QSharedPointer< ScriptCollection > ScriptCollectionFactory::createPlugin( const scriptobject_ptr& object, ScriptAccount* scriptAccount )
{
    const QVariantMap collectionInfo =  object->syncInvoke( "collection" ).toMap();

    if ( collectionInfo.isEmpty() ||
            !collectionInfo.contains( "prettyname" ) ||
            !collectionInfo.contains( "description" ) )
        return QSharedPointer< ScriptCollection >();

    const QString prettyname = collectionInfo.value( "prettyname" ).toString();
    const QString desc = collectionInfo.value( "description" ).toString();

    // at this point we assume that all the tracks browsable through a resolver belong to the local source
    Tomahawk::ScriptCollection* sc = new Tomahawk::ScriptCollection( object, SourceList::instance()->getLocal(), scriptAccount );
    QSharedPointer<ScriptCollection> collection( sc );
    collection->setWeakRef( collection.toWeakRef() );


    sc->setServiceName( prettyname );
    sc->setDescription( desc );

    if ( collectionInfo.contains( "trackcount" ) ) //a resolver might not expose this
    {
        bool ok = false;
        int trackCount = collectionInfo.value( "trackcount" ).toInt( &ok );
        if ( ok )
            sc->setTrackCount( trackCount );
    }

    if ( collectionInfo.contains( "iconfile" ) )
    {
        QString iconPath = QFileInfo( scriptAccount->filePath() ).path() + "/"
                            + collectionInfo.value( "iconfile" ).toString();

        QPixmap iconPixmap;
        bool ok = iconPixmap.load( iconPath );
        if ( ok && !iconPixmap.isNull() )
            sc->setIcon( iconPixmap );
    }

    sc->fetchIcon( collectionInfo.value( "iconurl" ).toString() );

    return collection;
}
