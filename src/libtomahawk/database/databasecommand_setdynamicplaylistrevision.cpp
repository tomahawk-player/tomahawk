#include "databasecommand_setdynamicplaylistrevision.h"

#include <QSqlQuery>

#include "tomahawksqlquery.h"
#include "dynamic/DynamicPlaylist.h"
#include "dynamic/DynamicControl.h"

DatabaseCommand_SetDynamicPlaylistRevision::DatabaseCommand_SetDynamicPlaylistRevision(const Tomahawk::source_ptr& s, 
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
{

}

DatabaseCommand_SetDynamicPlaylistRevision::DatabaseCommand_SetDynamicPlaylistRevision(const Tomahawk::source_ptr& s, 
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
{

}

QVariantList DatabaseCommand_SetDynamicPlaylistRevision::controlsV()
{
    if( m_controls.isEmpty()  )
        return m_controlsV;
    
    if( !m_controls.isEmpty() && m_controlsV.isEmpty() )
    {
        foreach( const dyncontrol_ptr& control, m_controls ) 
        {
            m_controlsV << QJson::QObjectHelper::qobject2qvariant( control.data() );
        }
    }
    return m_controlsV;
    
}


void
DatabaseCommand_SetDynamicPlaylistRevision::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;
    
    QStringList orderedentriesguids;
    foreach( const QVariant& v, orderedguids() )
        orderedentriesguids << v.toString();
    
    // private, but we are a friend. will recall itself in its own thread:
    dynplaylist_ptr playlist = source()->collection()->dynamicPlaylist( playlistguid() );
    
    if ( playlist.isNull() )
    {
        qDebug() << playlistguid();
        Q_ASSERT( !playlist.isNull() );
        return;
    }  
    if( m_mode == OnDemand )
        playlist->setRevision(  newrev(),
                                true, // this *is* the newest revision so far
                                m_type,
                                m_controls,
                                m_applied );
    else
        playlist->setRevision(  newrev(),
                                orderedentriesguids,
                                m_previous_rev_orderedguids,
                                m_type,
                                m_controls,
                                true, // this *is* the newest revision so far
                                m_addedmap,
                                m_applied );
    
    if( source()->isLocal() )
		Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_SetDynamicPlaylistRevision::exec( DatabaseImpl* lib )
{
    DatabaseCommand_SetPlaylistRevision::exec( lib );
    
    QVariantList newcontrols;
    foreach( const dyncontrol_ptr& control, m_controls ) {
        newcontrols << control->id();
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
    if( !delQuery.exec() )
		qWarning() << "Failed to delete controls from dynamic playlist controls table";
    
    TomahawkSqlQuery controlsQuery = lib->newquery();
    controlsQuery.prepare( "INSERT INTO dynamic_playlist_controls( id, playlist, selectedType, match, input ) "
                            "VALUES( ?, ?, ?, ?, ? )" );
    foreach( const dyncontrol_ptr& control, m_controls )
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
