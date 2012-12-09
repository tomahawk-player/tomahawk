/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "DatabaseCommand_SetDynamicPlaylistRevision.h"

#include "Source.h"
#include "DatabaseImpl.h"
#include "TomahawkSqlQuery.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include "playlist/dynamic/DynamicControl.h"
#include "network/Servent.h"
#include "utils/Logger.h"

#include <QSqlQuery>

DatabaseCommand_SetDynamicPlaylistRevision::DatabaseCommand_SetDynamicPlaylistRevision( const Tomahawk::source_ptr& s,
                                                                                const QString& playlistguid,
                                                                                const QString& newrev,
                                                                                const QString& oldrev,
                                                                                const QStringList& orderedguids,
                                                                                const QList< plentry_ptr >& addedentries,
                                                                                const QList<plentry_ptr>& entries,
                                                                                const QString& type,
                                                                                GeneratorMode mode,
                                                                                const QList< dyncontrol_ptr >& controls )
    : DatabaseCommand_SetPlaylistRevision( s, playlistguid, newrev, oldrev, orderedguids, addedentries, entries )
    , m_type( type )
    , m_mode( mode )
    , m_controls( controls )
    , m_playlist( 0 )
{

}


DatabaseCommand_SetDynamicPlaylistRevision::DatabaseCommand_SetDynamicPlaylistRevision( const Tomahawk::source_ptr& s,
                                                                                const QString& playlistguid,
                                                                                const QString& newrev,
                                                                                const QString& oldrev,
                                                                                const QString& type,
                                                                                GeneratorMode mode,
                                                                                const QList< dyncontrol_ptr >& controls )
    : DatabaseCommand_SetPlaylistRevision( s, playlistguid, newrev, oldrev, QStringList(), QList< plentry_ptr >(), QList< plentry_ptr >() )
    , m_type( type )
    , m_mode( mode )
    , m_controls( controls )
    , m_playlist( 0 )
{

}


QVariantList
DatabaseCommand_SetDynamicPlaylistRevision::controlsV()
{
    if ( m_controls.isEmpty() )
        return m_controlsV;

    if ( !m_controls.isEmpty() && m_controlsV.isEmpty() )
    {
        foreach ( const dyncontrol_ptr& control, m_controls )
        {
            m_controlsV << QJson::QObjectHelper::qobject2qvariant( control.data() );
        }
    }

    return m_controlsV;
}


void
DatabaseCommand_SetDynamicPlaylistRevision::postCommitHook()
{
    if ( source().isNull() || source()->collection().isNull() )
    {
        tDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }

    QStringList orderedentriesguids;
    foreach( const QVariant& v, orderedguids() )
        orderedentriesguids << v.toString();

    Q_ASSERT( !source().isNull() );
    Q_ASSERT( !source()->collection().isNull() );
    tLog() << "Postcommitting this playlist:" << playlistguid() << source().isNull();

    // private, but we are a friend. will recall itself in its own thread:
    dynplaylist_ptr playlist = source()->collection()->autoPlaylist( playlistguid() );
    if ( playlist.isNull() )
        playlist = source()->collection()->station( playlistguid() );
    // UGH we don't have a sharedptr from DynamicPlaylist+

    DynamicPlaylist* rawPl = playlist.data();
    if( playlist.isNull() ) // if it's neither an auto or station, it must not be auto-loaded, so we MUST have been told about it directly
        rawPl = m_playlist;

    if ( rawPl == 0 )
    {
        tLog() <<"Got null playlist with guid:" << playlistguid() << "from source and collection:" << source()->friendlyName() << source()->collection()->name() << "and mode is static?:" << (m_mode == Static);
        Q_ASSERT( false );
        return;
    }

    if ( !m_controlsV.isEmpty() && m_controls.isEmpty() )
    {
        QList<QVariantMap> controlMap;
        foreach( const QVariant& v, m_controlsV )
            controlMap << v.toMap();

        if ( m_mode == OnDemand )
            rawPl->setRevision(  newrev(),
                                    true, // this *is* the newest revision so far
                                    m_type,
                                    controlMap,
                                    m_applied );
        else
            rawPl->setRevision(  newrev(),
                                    orderedentriesguids,
                                    m_previous_rev_orderedguids,
                                    m_type,
                                    controlMap,
                                    true, // this *is* the newest revision so far
                                    m_addedmap,
                                    m_applied );
    }
    else
    {
        if ( m_mode == OnDemand )
            rawPl->setRevision(  newrev(),
                                    true, // this *is* the newest revision so far
                                    m_type,
                                    m_controls,
                                    m_applied );
        else
            rawPl->setRevision(  newrev(),
                                    orderedentriesguids,
                                    m_previous_rev_orderedguids,
                                    m_type,
                                    m_controls,
                                    true, // this *is* the newest revision so far
                                    m_addedmap,
                                    m_applied );
    }

    if ( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_SetDynamicPlaylistRevision::exec( DatabaseImpl* lib )
{
    DatabaseCommand_SetPlaylistRevision::exec( lib );

    QVariantList newcontrols;
    if ( m_controlsV.isEmpty() && !m_controls.isEmpty() )
    {
        foreach( const dyncontrol_ptr& control, m_controls )
        {
            newcontrols << control->id();
        }
    }
    else if( !m_controlsV.isEmpty() )
    {
        foreach( const QVariant& v, m_controlsV )
        {
            newcontrols << v.toMap().value( "id" );
        }
    }

    QJson::Serializer ser;
    const QByteArray newcontrols_data = ser.serialize( newcontrols );

    TomahawkSqlQuery query = lib->newquery();
    QString sql = "INSERT INTO dynamic_playlist_revision (guid, controls, plmode, pltype) "
                  "VALUES(?, ?, ?, ?)";

    query.prepare( sql );
    query.addBindValue( m_newrev );
    query.addBindValue( newcontrols_data );
    query.addBindValue( QString::number( (int) m_mode ) );
    query.addBindValue( m_type );
    query.exec();

    // delete all the old controls, replace with new onws
    qDebug() << "Deleting controls with playlist id" << m_playlistguid;
    TomahawkSqlQuery delQuery = lib->newquery();
    delQuery.prepare( "DELETE FROM dynamic_playlist_controls WHERE playlist = ?" );
    delQuery.addBindValue( m_playlistguid );
    if ( !delQuery.exec() )
        tLog() << "Failed to delete controls from dynamic playlist controls table";

    TomahawkSqlQuery controlsQuery = lib->newquery();
    controlsQuery.prepare( "INSERT INTO dynamic_playlist_controls( id, playlist, selectedType, match, input ) "
                            "VALUES( ?, ?, ?, ?, ? )" );
    if ( m_controlsV.isEmpty() && !m_controls.isEmpty() )
    {
        foreach ( const dyncontrol_ptr& control, m_controls )
        {
            qDebug() << "inserting dynamic control:" << control->id() << m_playlistguid << control->selectedType() << control->match() << control->input();
            controlsQuery.addBindValue( control->id() );
            controlsQuery.addBindValue( m_playlistguid );
            controlsQuery.addBindValue( control->selectedType() );
            controlsQuery.addBindValue( control->match() );
            controlsQuery.addBindValue( control->input() );

            controlsQuery.exec();
        }
    }
    else
    {
        foreach ( const QVariant& v, m_controlsV )
        {
            QVariantMap control = v.toMap();
            qDebug() << "inserting dynamic control from JSON:" << control.value( "id" ) << m_playlistguid << control.value( "selectedType" ) << control.value( "match" ) << control.value( "input" );
            controlsQuery.addBindValue( control.value( "id" ) );
            controlsQuery.addBindValue( m_playlistguid );
            controlsQuery.addBindValue( control.value( "selectedType" ) );
            controlsQuery.addBindValue( control.value( "match" ) );
            controlsQuery.addBindValue( control.value( "input" ) );

            controlsQuery.exec();
        }
    }

    if ( m_applied )
    {
        tLog() << "updating dynamic playlist, optimistic locking okay";

        TomahawkSqlQuery query2 = lib->newquery();
        query2.prepare( "UPDATE dynamic_playlist SET pltype = ?, plmode = ? WHERE guid = ?" );
        query2.bindValue( 0, m_type );
        query2.bindValue( 1, m_mode );
        query2.bindValue( 2, m_playlistguid );
        query2.exec();
    }
}

void
DatabaseCommand_SetDynamicPlaylistRevision::setPlaylist( DynamicPlaylist* pl )
{
    m_playlist = pl;
}
