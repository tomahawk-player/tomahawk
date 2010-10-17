#include "databasecommand_setplaylistrevision.h"

#include <QSqlQuery>

#include "tomahawksqlquery.h"
#include "tomahawk/tomahawkapp.h"


DatabaseCommand_SetPlaylistRevision::DatabaseCommand_SetPlaylistRevision(
                      const source_ptr& s,
                      QString playlistguid,
                      QString newrev,
                      QString oldrev,
                      QStringList orderedguids,
                      QList<plentry_ptr> addedentries )
    : DatabaseCommandLoggable( s )
    , m_newrev( newrev )
    , m_oldrev( oldrev )
    , m_addedentries( addedentries )
    , m_applied( false )
{
    setPlaylistguid( playlistguid );

    QVariantList tmp;
    foreach( const QString& s, orderedguids )
        tmp << s;

    setOrderedguids( tmp );
}


void
DatabaseCommand_SetPlaylistRevision::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;

    QStringList orderedentriesguids;
    foreach( const QVariant& v, m_orderedguids )
        orderedentriesguids << v.toString();

    // private, but we are a friend. will recall itself in its own thread:
    playlist_ptr playlist = source()->collection()->playlist( m_playlistguid );

    if ( playlist.isNull() )
    {
        qDebug() << m_playlistguid;
        Q_ASSERT( !playlist.isNull() );
        return;
    }

    playlist->setRevision( m_newrev,
                           orderedentriesguids,
                           m_previous_rev_orderedguids,
                           true, // this *is* the newest revision so far
                           m_addedmap,
                           m_applied );

    if( source()->isLocal() )
        APP->servent().triggerDBSync();
}


void
DatabaseCommand_SetPlaylistRevision::exec( DatabaseImpl* lib )
{
    using namespace Tomahawk;

    QString currentrevision;

    // get the current revision for this playlist
    // this also serves to check the playlist exists.
    TomahawkSqlQuery chkq = lib->newquery();
    chkq.prepare("SELECT currentrevision FROM playlist WHERE guid = ?");
    chkq.addBindValue( m_playlistguid );
    if( chkq.exec() && chkq.next() )
    {
        currentrevision = chkq.value( 0 ).toString();
        //qDebug() << Q_FUNC_INFO << "pl guid" << m_playlistguid << " curr rev" << currentrevision;
    }
    else
    {
        throw "No such playlist, WTF?";
        return;
    }

    QVariantList vlist = m_orderedguids;

    QJson::Serializer ser;
    const QByteArray entries = ser.serialize( vlist );

    // add any new items:
    TomahawkSqlQuery adde = lib->newquery();

    QString sql = "INSERT INTO playlist_item( guid, playlist, trackname, artistname, albumname, "
                                             "annotation, duration, addedon, addedby, result_hint ) "
                  "VALUES( ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )";
    adde.prepare( sql );

    qDebug() << "Num new playlist_items to add:" << m_addedentries.length();
    foreach( const plentry_ptr& e, m_addedentries )
    {
        m_addedmap.insert( e->guid(), e ); // needed in postcommithook

        adde.bindValue( 0, e->guid() );
        adde.bindValue( 1, m_playlistguid );
        adde.bindValue( 2, e->query()->track() );
        adde.bindValue( 3, e->query()->artist() );
        adde.bindValue( 4, e->query()->album() );
        adde.bindValue( 5, e->annotation() );
        adde.bindValue( 6, (int) e->duration() );
        adde.bindValue( 7, e->lastmodified() );
        adde.bindValue( 8, source()->isLocal() ? QVariant(QVariant::Int) : source()->id() );
        adde.bindValue( 9, "" );
        bool ok = adde.exec();
        if( !ok )
        {
            qDebug() << adde.lastError().databaseText() << adde.lastError().driverText() << "\n"
                     << sql << endl
                     << adde.boundValues().size() ;
            int i = 0;
            foreach(QVariant param, adde.boundValues()) qDebug() << i++ << param;
            Q_ASSERT( ok );
        }

    }

    // add the new revision:
    //qDebug() << "Adding new playlist revision, guid:" << m_newrev
    //         << entries;
    TomahawkSqlQuery query = lib->newquery();
    sql = "INSERT INTO playlist_revision(guid, playlist, entries, author, timestamp, previous_revision) "
                 "VALUES(?, ?, ?, ?, ?, ?)";

    query.prepare( sql );
    query.addBindValue( m_newrev );
    query.addBindValue( m_playlistguid );
    query.addBindValue( entries );
    query.addBindValue( source()->isLocal() ? QVariant(QVariant::Int) : source()->id() );
    query.addBindValue( 0 ); //ts
    query.addBindValue( m_oldrev.isEmpty() ? QVariant(QVariant::String) : m_oldrev );

    //qDebug() << sql << "\n" << query.boundValues();

    bool ok = query.exec();
    Q_ASSERT( ok );

    qDebug() << "Currentrevision:" << currentrevision << "oldrev:" << m_oldrev;
    // if optimistic locking is ok, update current revision to this new one
    if( currentrevision == m_oldrev )
    {
        TomahawkSqlQuery query2 = lib->newquery();
        qDebug() << "updating current revision, optimistic locking ok";
        query2.prepare("UPDATE playlist SET currentrevision = ? WHERE guid = ?");
        query2.bindValue( 0, m_newrev );
        query2.bindValue( 1, m_playlistguid );
        bool uok = query2.exec();
        Q_ASSERT( uok );
        m_applied = true;


        // load previous revision entries, which we need to pass on
        // so the change can be diffed
        TomahawkSqlQuery query_entries = lib->newquery();
        query_entries.prepare("SELECT entries, playlist, author, timestamp, previous_revision "
                              "FROM playlist_revision "
                              "WHERE guid = :guid");
        query_entries.bindValue( ":guid", m_oldrev );
        query_entries.exec();
        if( query_entries.next() )
        {
            // entries should be a list of strings:
            QJson::Parser parser;
            QVariant v = parser.parse( query_entries.value(0).toByteArray(), &ok );
            Q_ASSERT( ok && v.type() == QVariant::List ); //TODO
            m_previous_rev_orderedguids = v.toStringList();
        }
    }
    else
    {
        qDebug() << "Not updating current revision, optimistic locking fail";
    }

}
