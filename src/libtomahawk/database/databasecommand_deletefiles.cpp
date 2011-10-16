/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "databasecommand_deletefiles.h"

#include <QSqlQuery>

#include "artist.h"
#include "album.h"
#include "collection.h"
#include "source.h"
#include "database/database.h"
#include "databasecommand_collectionstats.h"
#include "databaseimpl.h"
#include "network/servent.h"
#include "utils/logger.h"
#include "utils/tomahawkutils.h"

using namespace Tomahawk;


// After changing a collection, we need to tell other bits of the system:
void
DatabaseCommand_DeleteFiles::postCommitHook()
{
    if ( !m_files.count() )
        return;

    // make the collection object emit its tracksAdded signal, so the
    // collection browser will update/fade in etc.
    Collection* coll = source()->collection().data();

    connect( this, SIGNAL( notify( QStringList ) ),
             coll,   SLOT( delTracks( QStringList ) ), Qt::QueuedConnection );

    tDebug() << "Notifying of deleted tracks:" << m_files.size();
    emit notify( m_files );

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_DeleteFiles::exec( DatabaseImpl* dbi )
{
//    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();
    TomahawkSqlQuery delquery = dbi->newquery();
    QString lastPath;

    if ( source()->isLocal() )
    {
        tDebug() << Q_FUNC_INFO << " source is local";
        if ( m_dir.path() != QString( "." ) )
        {
            tDebug() << "Deleting" << m_dir.path() << "from db for localsource" << srcid;
            TomahawkSqlQuery dirquery = dbi->newquery();
            QString path( "file://" + m_dir.canonicalPath() + "/%" );
            dirquery.prepare( QString( "SELECT id, url FROM file WHERE source IS NULL AND url LIKE '%1'" ).arg( TomahawkUtils::sqlEscape( path ) ) );
            dirquery.exec();

            while ( dirquery.next() )
            {
                QFileInfo fi( dirquery.value( 1 ).toString().mid( 7 ) ); // remove file://
                if ( fi.canonicalPath() != m_dir.canonicalPath() )
                {
                    if ( lastPath != fi.canonicalPath() )
                        tDebug() << "Skipping subdir:" << fi.canonicalPath();

                    lastPath = fi.canonicalPath();
                    continue;
                }

                m_files << dirquery.value( 1 ).toString();
                m_ids << dirquery.value( 0 ).toUInt();
            }
        }
        else if ( !m_ids.isEmpty() )
        {
            tDebug() << Q_FUNC_INFO << " deleting given ids";
            TomahawkSqlQuery dirquery = dbi->newquery();

            QString idstring;
            foreach( const QVariant& id, m_ids )
                idstring.append( id.toString() + ", " );
            idstring.chop( 2 ); //remove the trailing ", "

            dirquery.prepare( QString( "SELECT id, url FROM file WHERE source IS NULL AND id IN ( %1 )" ).arg( idstring ) );
            
            dirquery.exec();
            while ( dirquery.next() )
            {
                //tDebug() << Q_FUNC_INFO << " found dirquery value 0: " << dirquery.value( 0 ).toString() << " and value 1: " << dirquery.value( 1 ).toString();
                m_files << dirquery.value( 1 ).toString();
            }

            //tDebug() << Q_FUNC_INFO << " executed query was: " << dirquery.executedQuery();
            //tDebug() << Q_FUNC_INFO << " files selected for delete: " << m_files;
        }
        else if ( m_deleteAll )
        {
            TomahawkSqlQuery dirquery = dbi->newquery();
            
            dirquery.prepare( QString( "SELECT id, url FROM file WHERE source IS NULL" ) );
            
            dirquery.exec();
            while ( dirquery.next() )
            {
                m_ids << dirquery.value( 0 ).toString();
                m_files << dirquery.value( 1 ).toString();
            }
        }
    }
    else
    {
        if ( m_deleteAll )
        {
            TomahawkSqlQuery dirquery = dbi->newquery();
            
            dirquery.prepare( QString( "SELECT url FROM file WHERE source = %1" ).arg( source()->id() ) );
            
            dirquery.exec();
            while ( dirquery.next() )
                m_ids << dirquery.value( 0 ).toString();
        }

        foreach( const QVariant& id, m_ids )
            m_files << QString( "servent://%1\t%2" ).arg( source()->userName() ).arg( id.toString() );
    }

    if ( m_deleteAll )
    {
        if ( !m_ids.isEmpty() )
        {
            delquery.prepare( QString( "DELETE FROM file WHERE source %1" )
            .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );

            if( !delquery.exec() )
            {
                qDebug() << "Failed to delete file:"
                << delquery.lastError().databaseText()
                << delquery.lastError().driverText()
                << delquery.boundValues();
            }
        }
    }
    else if ( !m_ids.isEmpty() )
    {
        tDebug() << Q_FUNC_INFO << " executing delete";
        delquery.prepare( QString( "DELETE FROM file WHERE source %1 AND %2 IN ( ? )" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) )
                             .arg( source()->isLocal() ? "id" : "url"  ) );

        QString idstring;
        foreach( const QVariant& id, m_ids )
                idstring.append( id.toString() + ", " );
        idstring.chop( 2 ); //remove the trailing ", "

        delquery.prepare( QString( "DELETE FROM file WHERE source %1 AND %2 IN ( %3 )" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) )
                             .arg( source()->isLocal() ? "id" : "url"  )
                             .arg( idstring ) );
        
        if( !delquery.exec() )
        {
            qDebug() << "Failed to delete file:"
                << delquery.lastError().databaseText()
                << delquery.lastError().driverText()
                << delquery.boundValues();
        }

        //tDebug() << Q_FUNC_INFO << " executed query was: " << delquery.executedQuery();
    }

    emit done( m_files, source()->collection() );
}
