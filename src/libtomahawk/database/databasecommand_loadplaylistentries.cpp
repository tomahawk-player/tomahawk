#include "databasecommand_loadplaylistentries.h"

#include <QSqlQuery>

#include "databaseimpl.h"

using namespace Tomahawk;


void
DatabaseCommand_LoadPlaylistEntries::exec( DatabaseImpl* dbi )
{
    qDebug() << "Loading playlist entries for revision" << m_revguid;
    generateEntries( dbi );
    
    emit done( m_revguid, m_guids, m_oldentries, m_islatest, m_entrymap, true );
}

void DatabaseCommand_LoadPlaylistEntries::generateEntries( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query_entries = dbi->newquery();
    query_entries.prepare("SELECT entries, playlist, author, timestamp, previous_revision "
                          "FROM playlist_revision "
                          "WHERE guid = :guid");
    query_entries.bindValue( ":guid", m_revguid );
    query_entries.exec();
    
    qDebug() << "trying to load entries:" << m_revguid;
    QString prevrev;
    QJson::Parser parser; bool ok;
    
    if( query_entries.next() )
    {
        // entries should be a list of strings:
        QVariant v = parser.parse( query_entries.value(0).toByteArray(), &ok );
        Q_ASSERT( ok && v.type() == QVariant::List ); //TODO
        m_guids = v.toStringList();
        //        qDebug() << "Entries:" << guids;
        
        QString inclause = QString("('%1')").arg(m_guids.join("', '"));
        
        TomahawkSqlQuery query = dbi->newquery();
        QString sql = QString("SELECT guid, trackname, artistname, albumname, annotation, "
                              "duration, addedon, addedby, result_hint "
                              "FROM playlist_item "
                              "WHERE guid IN %1").arg( inclause );
        //qDebug() << sql;
        
        query.exec( sql );
        while( query.next() )
        {
            plentry_ptr e( new PlaylistEntry );
            e->setGuid( query.value( 0 ).toString() );
            e->setAnnotation( query.value( 4 ).toString() );
            e->setDuration( query.value( 5 ).toUInt() );
            e->setLastmodified( 0 ); // TODO e->lastmodified = query.value(6).toInt();
            e->setResultHint( query.value( 8 ).toString() );
            
            QVariantMap m;
            m.insert( "artist", query.value( 2 ).toString() );
            m.insert( "album", query.value( 3 ).toString() );
            m.insert( "track", query.value( 1 ).toString() );
            m.insert( "resulthint", query.value( 8 ).toString() );
            m.insert( "qid", uuid() );

            Tomahawk::query_ptr q = Tomahawk::Query::get( m, false );
            e->setQuery( q );
            
            m_entrymap.insert( e->guid(), e );
        }
        
        prevrev = query_entries.value( 4 ).toString();

    }
    else
    {
        qDebug() << "Playlist has no current revision data";
    }
    
    if( prevrev.length() )
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
        if( !query_entries_old.next() )
        {
            return;
            Q_ASSERT( false );
        }
        
        QVariant v = parser.parse( query_entries_old.value( 0 ).toByteArray(), &ok );
        Q_ASSERT( ok && v.type() == QVariant::List ); //TODO
        m_oldentries = v.toStringList();
        m_islatest = query_entries_old.value( 1 ).toBool();
    }
    
    qDebug() << Q_FUNC_INFO << "entrymap:" << m_entrymap;
}
