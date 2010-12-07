#include "databasecommand_createdynamicplaylist.h"

#include <QSqlQuery>

#include "tomahawk/tomahawkapp.h"
#include "dynamic/dynamicplaylist.h"
#include "dynamic/dynamiccontrol.h"
#include "dynamic/generatorinterface.h"

using namespace Tomahawk;


DatabaseCommand_CreateDynamicPlaylist::DatabaseCommand_CreateDynamicPlaylist( QObject* parent )
: DatabaseCommand_CreatePlaylist( parent )
{
    qDebug() << Q_FUNC_INFO << "creating dynamiccreatecommand 1";
}


DatabaseCommand_CreateDynamicPlaylist::DatabaseCommand_CreateDynamicPlaylist( const source_ptr& author,
                                                                const dynplaylist_ptr& playlist )
    : DatabaseCommand_CreatePlaylist( author, playlist.staticCast<Playlist>() )
    , m_playlist( playlist )
{
    qDebug() << Q_FUNC_INFO << "creating dynamiccreatecommand 2";
}


void
DatabaseCommand_CreateDynamicPlaylist::exec( DatabaseImpl* lib )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !m_playlist.isNull() );
    Q_ASSERT( !source().isNull() );
    
    DatabaseCommand_CreatePlaylist::exec( lib );
    qDebug() << "Created normal playlist, now creating additional dynamic info!" << m_playlist.isNull();
    
    TomahawkSqlQuery cre = lib->newquery();
    cre.prepare( "INSERT INTO dynamic_playlist( guid, pltype, plmode ) "
                 "VALUES( ?, ?, ? )" );
    cre.bindValue( 0, m_playlist->guid() );
    cre.bindValue( 1, m_playlist->type() );
    cre.bindValue( 2, m_playlist->mode() );
    
    qDebug() << "CREATE DYNPLAYLIST:" << cre.boundValues();
    
    cre.exec();
    
    // save the controls
    cre = lib->newquery();
    cre.prepare( "INSERT INTO dynamic_playlist_controls( id, selectedType, match, input) "
                 "VALUES( :id, :selectedType, :match, :input )" );
    foreach( const dyncontrol_ptr& control, m_playlist->generator()->controls() ) {
    
        cre.bindValue( ":id", control->id() );
        cre.bindValue( ":selectedType", control->selectedType() );
        cre.bindValue( ":match", control->match() );
        cre.bindValue( ":input", control->input() );
        
        qDebug() << "CREATE DYNPLAYLIST CONTROL:" << cre.boundValues();
        
        cre.exec();
    }
}


void
DatabaseCommand_CreateDynamicPlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;
    if( report() == false )
        return;
    
    qDebug() << Q_FUNC_INFO << "..reporting..";
    m_playlist->reportCreated( m_playlist );
    
    if( source()->isLocal() )
        APP->servent().triggerDBSync();
}
