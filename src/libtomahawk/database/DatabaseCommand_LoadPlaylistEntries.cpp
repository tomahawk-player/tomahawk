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

#include "DatabaseCommand_LoadPlaylistEntries.h"

#include <QSqlQuery>

#include "DatabaseImpl.h"
#include "Query.h"
#include "qjson/parser.h"
#include "utils/Logger.h"
#include "Source.h"

using namespace Tomahawk;


void
DatabaseCommand_LoadPlaylistEntries::exec( DatabaseImpl* dbi )
{
    generateEntries( dbi );

    emit done( m_revguid, m_guids, m_oldentries, m_islatest, m_entrymap, true );
}


void
DatabaseCommand_LoadPlaylistEntries::generateEntries( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query_entries = dbi->newquery();
    query_entries.prepare( "SELECT entries, playlist, author, timestamp, previous_revision "
                           "FROM playlist_revision "
                           "WHERE guid = :guid" );
    query_entries.bindValue( ":guid", m_revguid );
    query_entries.exec();

    tLog( LOGVERBOSE ) << "trying to load playlist entries for guid:" << m_revguid;
    QString prevrev;
    QJson::Parser parser; bool ok;

    if ( query_entries.next() )
    {
        if ( !query_entries.value( 0 ).isNull() )
        {
            // entries should be a list of strings:
            QVariant v = parser.parse( query_entries.value( 0 ).toByteArray(), &ok );
            Q_ASSERT( ok && v.type() == QVariant::List ); //TODO

            m_guids = v.toStringList();
            QString inclause = QString( "('%1')" ).arg( m_guids.join( "', '" ) );

            TomahawkSqlQuery query = dbi->newquery();
            QString sql = QString( "SELECT guid, trackname, artistname, albumname, annotation, "
                                "duration, addedon, addedby, result_hint "
                                "FROM playlist_item "
                                "WHERE guid IN %1" ).arg( inclause );

            query.exec( sql );
            while ( query.next() )
            {
                plentry_ptr e( new PlaylistEntry );
                e->setGuid( query.value( 0 ).toString() );
                e->setAnnotation( query.value( 4 ).toString() );
                e->setDuration( query.value( 5 ).toUInt() );
                e->setLastmodified( 0 ); // TODO e->lastmodified = query.value( 6 ).toInt();
                const QString resultHint = query.value( 8 ).toString();
                e->setResultHint( resultHint );

                Tomahawk::query_ptr q = Tomahawk::Query::get( query.value( 2 ).toString(), query.value( 1 ).toString(), query.value( 3 ).toString() );
                if ( q.isNull() )
                    continue;

                q->setResultHint( resultHint );
                if ( resultHint.startsWith( "http" ) )
                    q->setSaveHTTPResultHint( true );

                q->setProperty( "annotation", e->annotation() );
                e->setQuery( q );

                m_entrymap.insert( e->guid(), e );
            }
        }

        prevrev = query_entries.value( 4 ).toString();
    }
    else
    {
//        qDebug() << "Playlist has no current revision data";
    }

    if ( prevrev.length() )
    {
        TomahawkSqlQuery query_entries_old = dbi->newquery();
        query_entries_old.prepare( "SELECT entries, "
                                   "(SELECT currentrevision = ? FROM playlist WHERE guid = ?) "
                                   "FROM playlist_revision "
                                   "WHERE guid = ?" );
        query_entries_old.addBindValue( m_revguid );
        query_entries_old.addBindValue( query_entries.value( 1 ).toString() );
        query_entries_old.addBindValue( prevrev );

        query_entries_old.exec();
        if ( !query_entries_old.next() )
        {
            return;
            Q_ASSERT( false );
        }

        if ( !query_entries_old.value( 0 ).isNull() )
        {
            QVariant v = parser.parse( query_entries_old.value( 0 ).toByteArray(), &ok );
            Q_ASSERT( ok && v.type() == QVariant::List ); //TODO
            m_oldentries = v.toStringList();
        }
        m_islatest = query_entries_old.value( 1 ).toBool();
    }

//    qDebug() << Q_FUNC_INFO << "entrymap:" << m_entrymap;
}
