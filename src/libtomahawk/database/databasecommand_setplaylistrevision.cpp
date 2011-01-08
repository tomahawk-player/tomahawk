#include "databasecommand_setplaylistrevision.h"

#include <QSqlQuery>

#include "tomahawksqlquery.h"
#include "network/servent.h"


DatabaseCommand_SetPlaylistRevision::DatabaseCommand_SetPlaylistRevision(
                      const source_ptr& s,
                      const QString& playlistguid,
                      const QString& newrev,
                      const QString& oldrev,
                      const QStringList& orderedguids,
                      const QList<plentry_ptr>& addedentries )
: DatabaseCommandLoggable( s )
    , m_applied( false )
    , m_newrev( newrev )
    , m_oldrev( oldrev )
    , m_addedentries( addedentries )
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
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_SetPlaylistRevision::exec( DatabaseImpl* lib )
{
    using namespace Tomahawk;

    // get the current revision for this playlist
    // this also serves to check the playlist exists.
    TomahawkSqlQuery chkq = lib->newquery();
    chkq.prepare("SELECT currentrevision FROM playlist WHERE guid = ?");
    chkq.addBindValue( m_playlistguid );
    if( chkq.exec() && chkq.next() )
    {
        m_currentRevision = chkq.value( 0 ).toString();
        qDebug() << Q_FUNC_INFO << "pl guid" << m_playlistguid << " curr rev" << m_currentRevision;
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
        adde.exec();
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
    query.exec();

    qDebug() << "Currentrevision:" << m_currentRevision << "oldrev:" << m_oldrev;
    // if optimistic locking is ok, update current revision to this new one
    if( m_currentRevision == m_oldrev )
    {
        TomahawkSqlQuery query2 = lib->newquery();
        qDebug() << "updating current revision, optimistic locking ok";
        query2.prepare("UPDATE playlist SET currentrevision = ? WHERE guid = ?");
        query2.bindValue( 0, m_newrev );
        query2.bindValue( 1, m_playlistguid );
        query2.exec();

        m_applied = true;

        // load previous revision entries, which we need to pass on
        // so the change can be diffed
        TomahawkSqlQuery query_entries = lib->newquery();
        query_entries.prepare( "SELECT entries, playlist, author, timestamp, previous_revision "
                               "FROM playlist_revision "
                               "WHERE guid = :guid" );
        query_entries.bindValue( ":guid", m_oldrev );
        query_entries.exec();
        if( query_entries.next() )
        {
            // entries should be a list of strings:
            bool ok;
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
