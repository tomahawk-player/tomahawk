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

#include "DatabaseCommand_SetPlaylistRevision.h"

#include <QSqlQuery>

#include "Source.h"
#include "DatabaseImpl.h"
#include "TomahawkSqlQuery.h"
#include "network/Servent.h"
#include "utils/Logger.h"

#include <qjson/serializer.h>
#include <qjson/parser.h>

using namespace Tomahawk;


DatabaseCommand_SetPlaylistRevision::DatabaseCommand_SetPlaylistRevision(
                      const source_ptr& s,
                      const QString& playlistguid,
                      const QString& newrev,
                      const QString& oldrev,
                      const QStringList& orderedguids,
                      const QList<plentry_ptr>& addedentries,
                      const QList<plentry_ptr>& entries )
    : DatabaseCommandLoggable( s )
    , m_failed( false )
    , m_applied( false )
    , m_newrev( newrev )
    , m_oldrev( oldrev )
    , m_addedentries( addedentries )
    , m_entries( entries )
    , m_metadataUpdate( false )
{
    Q_ASSERT( !newrev.isEmpty() );
    m_localOnly = ( newrev == oldrev );

    setPlaylistguid( playlistguid );

    QVariantList tmp;
    foreach( const QString& s, orderedguids )
        tmp << s;

    setOrderedguids( tmp );
}


DatabaseCommand_SetPlaylistRevision::DatabaseCommand_SetPlaylistRevision(
                        const source_ptr& s,
                        const QString& playlistguid,
                        const QString& newrev,
                        const QString& oldrev,
                        const QStringList& orderedguids,
                        const QList<plentry_ptr>& entriesToUpdate )
    : DatabaseCommandLoggable( s )
    , m_failed( false )
    , m_applied( false )
    , m_newrev( newrev )
    , m_oldrev( oldrev )
    , m_entries( entriesToUpdate )
    , m_metadataUpdate( true )
{
    Q_ASSERT( !newrev.isEmpty() );
    m_localOnly = false;

    QVariantList tmp;
    foreach( const QString& s, orderedguids )
        tmp << s;

    setOrderedguids( tmp );
    setPlaylistguid( playlistguid );
}


void
DatabaseCommand_SetPlaylistRevision::postCommitHook()
{
    tDebug() << Q_FUNC_INFO;
    if ( m_localOnly )
        return;

    QStringList orderedentriesguids;
    foreach( const QVariant& v, m_orderedguids )
        orderedentriesguids << v.toString();

    // private, but we are a friend. will recall itself in its own thread:
    playlist_ptr playlist = source()->dbCollection()->playlist( m_playlistguid );
//    Q_ASSERT( !playlist.isNull() );
    if ( !playlist )
        return;

    if ( playlist->loaded() )
    {
        playlist->setRevision( m_newrev,
                            orderedentriesguids,
                            m_previous_rev_orderedguids,
                            true, // this *is* the newest revision so far
                            m_addedmap,
                            m_applied );
    }
    else
        playlist->setCurrentrevision( m_newrev );

    if ( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_SetPlaylistRevision::exec( DatabaseImpl* lib )
{
    QString currentRevision;
    // get the current revision for this playlist
    // this also serves to check the playlist exists.
    TomahawkSqlQuery chkq = lib->newquery();
    chkq.prepare( "SELECT currentrevision FROM playlist WHERE guid = ?" );
    chkq.addBindValue( m_playlistguid );
    if ( chkq.exec() && chkq.next() )
    {
        currentRevision = chkq.value( 0 ).toString();
        tDebug() << Q_FUNC_INFO << "pl guid" << m_playlistguid << "- curr rev" << currentRevision;
    }
    else
    {
        tDebug() << "ERROR: No such playlist:" << m_playlistguid << currentRevision << source()->friendlyName() << source()->id();
//        Q_ASSERT_X( false, "DatabaseCommand_SetPlaylistRevision::exec", "No such playlist, WTF?" );
        m_failed = true;
        return;
    }

    QVariantList vlist = m_orderedguids;
    QJson::Serializer ser;
    const QByteArray entries = ser.serialize( vlist );

    // add any new items:
    TomahawkSqlQuery adde = lib->newquery();
    if ( m_localOnly )
    {
        QString sql = "UPDATE playlist_item SET result_hint = ? WHERE guid = ?";
        adde.prepare( sql );

        foreach( const plentry_ptr& e, m_entries )
        {
            if ( !e->isValid() )
                continue;
            if ( !e->query()->numResults() )
                continue;

            adde.bindValue( 0, e->resultHint() );
            adde.bindValue( 1, e->guid() );
            adde.exec();
        }

        return;
    }
    else if ( m_metadataUpdate )
    {
        QString sql = "UPDATE playlist_item SET trackname = ?, artistname = ?, albumname = ?, annotation = ?, duration = ?, addedon = ?, addedby = ? WHERE guid = ?";
        adde.prepare( sql );

        foreach( const plentry_ptr& e, m_entries )
        {
            if ( !e->isValid() )
                continue;

            adde.bindValue( 0, e->query()->queryTrack()->track() );
            adde.bindValue( 1, e->query()->queryTrack()->artist() );
            adde.bindValue( 2, e->query()->queryTrack()->album() );
            adde.bindValue( 3, e->annotation() );
            adde.bindValue( 4, (int) e->duration() );
            adde.bindValue( 5, e->lastmodified() );
            adde.bindValue( 6, source()->isLocal() ? QVariant(QVariant::Int) : source()->id() );
            adde.bindValue( 7, e->guid() );

            adde.exec();
        }
    }
    else
    {
        QString sql = "REPLACE INTO playlist_item( guid, playlist, trackname, artistname, albumname, "
                                                 "annotation, duration, addedon, addedby, result_hint ) "
                      "VALUES( ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )";
        adde.prepare( sql );

//         qDebug() << "Num new playlist_items to add:" << m_addedentries.length();
        foreach( const plentry_ptr& e, m_addedentries )
        {
            if ( !e->isValid() )
                continue;

//             qDebug() << "Adding:" << e->guid() << e->query()->track() << e->query()->artist();

            m_addedmap.insert( e->guid(), e ); // needed in postcommithook

            adde.bindValue( 0, e->guid() );
            adde.bindValue( 1, m_playlistguid );
            adde.bindValue( 2, e->query()->queryTrack()->track() );
            adde.bindValue( 3, e->query()->queryTrack()->artist() );
            adde.bindValue( 4, e->query()->queryTrack()->album() );
            adde.bindValue( 5, e->annotation() );
            adde.bindValue( 6, (int) e->duration() );
            adde.bindValue( 7, e->lastmodified() );
            adde.bindValue( 8, source()->isLocal() ? QVariant(QVariant::Int) : source()->id() );
            adde.bindValue( 9, e->resultHint() );
            adde.exec();
        }
    }

    // add / update the revision:
    TomahawkSqlQuery query = lib->newquery();
    QString sql = "INSERT INTO playlist_revision(guid, playlist, entries, author, timestamp, previous_revision) "
                  "VALUES(?, ?, ?, ?, ?, ?)";
    query.prepare( sql );

    query.addBindValue( m_newrev );
    query.addBindValue( m_playlistguid );
    query.addBindValue( entries );
    query.addBindValue( source()->isLocal() ? QVariant(QVariant::Int) : source()->id() );
    query.addBindValue( 0 ); //ts
    query.addBindValue( m_oldrev.isEmpty() ? QVariant(QVariant::String) : m_oldrev );
    query.exec();

    tDebug() << "Currentrevision:" << currentRevision << "oldrev:" << m_oldrev;
    // if optimistic locking is ok, update current revision to this new one
    if ( currentRevision == m_oldrev )
    {
        tDebug() << "Updating current revision, optimistic locking ok" << m_newrev;

        TomahawkSqlQuery query2 = lib->newquery();
        query2.prepare( "UPDATE playlist SET currentrevision = ? WHERE guid = ?" );
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
        if ( query_entries.next() )
        {
            bool ok;
            QJson::Parser parser;
            QVariant v = parser.parse( query_entries.value( 0 ).toByteArray(), &ok );
            Q_ASSERT( ok && v.type() == QVariant::List ); //TODO

            m_previous_rev_orderedguids = v.toStringList();
        }
    }
    else if ( !m_oldrev.isEmpty() )
    {
        tDebug() << "Not updating current revision, optimistic locking fail" << currentRevision << m_oldrev;

        Q_ASSERT( !source()->isLocal() );
    }
}
