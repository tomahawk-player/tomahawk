/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "DatabaseCommand_DeleteFiles.h"

#include <QtSql/QSqlQuery>

#include "Artist.h"
#include "Album.h"
#include "collection/Collection.h"
#include "Source.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "network/Servent.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"

using namespace Tomahawk;


// After changing a collection, we need to tell other bits of the system:
void
DatabaseCommand_DeleteFiles::postCommitHook()
{
    if ( !m_idList.count() )
        return;

    // make the collection object emit its tracksAdded signal, so the
    // collection browser will update/fade in etc.
    Collection* coll = source()->dbCollection().data();

    connect( this, SIGNAL( notify( QList<unsigned int> ) ),
             coll,   SLOT( delTracks( QList<unsigned int> ) ), Qt::QueuedConnection );

    tDebug() << "Notifying of deleted tracks:" << m_idList.size() << "from source" << source()->id();
    emit notify( m_idList );

    if ( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_DeleteFiles::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( !source().isNull() );

    int srcid = source()->isLocal() ? 0 : source()->id();
    TomahawkSqlQuery delquery = dbi->newquery();

    if ( m_deleteAll )
    {
        TomahawkSqlQuery dirquery = dbi->newquery();
        dirquery.prepare( QString( "SELECT id FROM file WHERE source %1" )
                    .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );
        dirquery.exec();
        
        while ( dirquery.next() )
            m_idList << dirquery.value( 0 ).toUInt();
    }
    else if ( source()->isLocal() )
    {
        if ( m_dir.path() != QString( "." ) )
        {
            tDebug() << "Deleting" << m_dir.path() << "from db for localsource" << srcid;
            TomahawkSqlQuery dirquery = dbi->newquery();
            QString path( "file://" + m_dir.canonicalPath() + "/%" );
            dirquery.prepare( QString( "SELECT id FROM file WHERE source IS NULL AND url LIKE '%1'" ).arg( TomahawkSqlQuery::escape( path ) ) );
            dirquery.exec();

            while ( dirquery.next() )
            {
                m_ids << dirquery.value( 0 );
                m_idList << dirquery.value( 0 ).toUInt();
            }
        }
        else if ( !m_ids.isEmpty() )
        {
            tDebug() << Q_FUNC_INFO << "deleting given ids";
            foreach ( const QVariant& id, m_ids )
                m_idList << id.toUInt();
        }
    }

    if ( m_deleteAll )
    {
        delquery.prepare( QString( "DELETE FROM file WHERE source %1" )
                    .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );
        delquery.exec();
    }
    else if ( !m_ids.isEmpty() )
    {
        QString idstring;
        foreach ( const QVariant& id, m_ids )
            idstring.append( id.toString() + ", " );
        idstring.chop( 2 ); //remove the trailing ", "

        if ( !source()->isLocal() )
        {
            delquery.prepare( QString( "SELECT id FROM file WHERE source = %1 AND url IN ( %2 )" )
                        .arg( source()->id() )
                        .arg( idstring ) );
            delquery.exec();
            
            idstring = QString();
            while ( delquery.next() )
            {
                idstring.append( delquery.value( 0 ).toString() + ", " );
                m_idList << delquery.value( 0 ).toUInt();
            }
            idstring.chop( 2 ); //remove the trailing ", "
        }

        delquery.prepare( QString( "DELETE FROM file WHERE source %1 AND id IN ( %2 )" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) )
                             .arg( idstring ) );
        delquery.exec();
    }

    if ( m_idList.count() )
        source()->updateIndexWhenSynced();

    emit done( m_idList, source()->dbCollection() );
}
