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

    emit notify( m_files );

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_DeleteFiles::exec( DatabaseImpl* dbi )
{
//    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    int deleted = 0;
    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();
    TomahawkSqlQuery delquery = dbi->newquery();
    QString lastPath;

    if ( m_dir.path() != QString( "." ) && source()->isLocal() )
    {
        qDebug() << "Deleting" << m_dir.path() << "from db for localsource" << srcid;
        TomahawkSqlQuery dirquery = dbi->newquery();

        dirquery.prepare( QString( "SELECT id, url FROM file WHERE source %1 AND url LIKE ?" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );
        delquery.prepare( QString( "DELETE FROM file WHERE source %1 AND id = ?" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );

        dirquery.bindValue( 0, "file://" + m_dir.canonicalPath() + "/%" );
        dirquery.exec();

        while ( dirquery.next() )
        {
            QFileInfo fi( dirquery.value( 1 ).toString().mid( 7 ) ); // remove file://
            if ( fi.canonicalPath() != m_dir.canonicalPath() )
            {
                if ( lastPath != fi.canonicalPath() )
                    qDebug() << "Skipping subdir:" << fi.canonicalPath();

                lastPath = fi.canonicalPath();
                continue;
            }

            m_ids << dirquery.value( 0 ).toUInt();
            m_files << dirquery.value( 1 ).toString();
        }

        foreach ( const QVariant& id, m_ids )
        {
            delquery.bindValue( 0, id.toUInt() );
            if( !delquery.exec() )
            {
                qDebug() << "Failed to delete file:"
                    << delquery.lastError().databaseText()
                    << delquery.lastError().driverText()
                    << delquery.boundValues();
                continue;
            }

            deleted++;
        }
    }
    else if ( !m_ids.isEmpty() && source()->isLocal() )
    {
        delquery.prepare( QString( "DELETE FROM file WHERE source %1 AND url = ?" )
                             .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) ) );

        foreach( const QVariant& id, m_ids )
        {
//            qDebug() << "Deleting" << id.toUInt() << "from db for source" << srcid;

            const QString url = QString( "servent://%1\t%2" ).arg( source()->userName() ).arg( id.toString() );
            m_files << url;

            delquery.bindValue( 0, id.toUInt() );
            if( !delquery.exec() )
            {
                qDebug() << "Failed to delete file:"
                    << delquery.lastError().databaseText()
                    << delquery.lastError().driverText()
                    << delquery.boundValues();
                continue;
            }

            deleted++;
        }
    }

//    qDebug() << "Deleted" << deleted << m_ids << m_files;

    emit done( m_files, source()->collection() );
}
