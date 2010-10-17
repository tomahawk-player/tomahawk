#include "databasecommand_loadplaylistentries.h"

#include <QSqlQuery>

#include "databaseimpl.h"

using namespace Tomahawk;


void
DatabaseCommand_LoadPlaylistEntries::exec( DatabaseImpl* dbi )
{
    qDebug() << "Loading playlist entries for revision" << m_guid;

    TomahawkSqlQuery query_entries = dbi->newquery();
    query_entries.prepare("SELECT entries, playlist, author, timestamp, previous_revision "
                          "FROM playlist_revision "
                          "WHERE guid = :guid");
    query_entries.bindValue( ":guid", m_guid );

    bool aok = query_entries.exec();
    Q_ASSERT( aok );

    QStringList guids;
    QMap< QString, plentry_ptr > entrymap;
    bool islatest = true;
    QStringList oldentries;
    QString prevrev;
    QJson::Parser parser; bool ok;

    if( query_entries.next() )
    {
        // entries should be a list of strings:
        QVariant v = parser.parse( query_entries.value(0).toByteArray(), &ok );
        Q_ASSERT( ok && v.type() == QVariant::List ); //TODO
        guids = v.toStringList();
//        qDebug() << "Entries:" << guids;

        QString inclause = QString("('%1')").arg(guids.join("', '"));

        TomahawkSqlQuery query = dbi->newquery();
        QString sql = QString("SELECT guid, trackname, artistname, albumname, annotation, "
                              "duration, addedon, addedby, result_hint "
                              "FROM playlist_item "
                              "WHERE guid IN %1").arg( inclause );
        //qDebug() << sql;

        bool xok = query.exec( sql );
        Q_ASSERT( xok );

        while( query.next() )
        {
            plentry_ptr e( new PlaylistEntry );
            e->setGuid( query.value( 0 ).toString() );
            e->setAnnotation( query.value( 4 ).toString() );
            e->setDuration( query.value( 5 ).toUInt() );
            e->setLastmodified( 0 ); // TODO e->lastmodified = query.value(6).toInt();
            e->setResulthint( query.value( 8 ).toString() );

            QVariantMap m;
            m.insert( "artist", query.value( 2 ).toString() );
            m.insert( "album", query.value( 3 ).toString() );
            m.insert( "track", query.value( 1 ).toString() );
            m.insert( "qid", uuid() );

            Tomahawk::query_ptr q( new Tomahawk::Query( m ) );
            e->setQuery( q );

            entrymap.insert( e->guid(), e );
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
        query_entries_old.addBindValue( m_guid );
        query_entries_old.addBindValue( query_entries.value( 1 ).toString() );
        query_entries_old.addBindValue( prevrev );
        bool ex = query_entries_old.exec();
        Q_ASSERT( ex );

        if( !query_entries_old.next() )
        {
            Q_ASSERT( false );
        }

        QVariant v = parser.parse( query_entries_old.value( 0 ).toByteArray(), &ok );
        Q_ASSERT( ok && v.type() == QVariant::List ); //TODO
        oldentries = v.toStringList();
        islatest = query_entries_old.value( 1 ).toBool();
    }

    qDebug() << Q_FUNC_INFO << "entrymap:" << entrymap;

    emit done( m_guid, guids, oldentries, islatest, entrymap, true );
}
