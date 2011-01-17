#include "databasecommand_createplaylist.h"

#include <QSqlQuery>

#include "network/servent.h"
#include "tomahawk/tomahawkapp.h"

using namespace Tomahawk;


DatabaseCommand_CreatePlaylist::DatabaseCommand_CreatePlaylist( QObject* parent )
    : DatabaseCommandLoggable( parent )
    , m_report( true )
{
    qDebug() << Q_FUNC_INFO << "def";
}


DatabaseCommand_CreatePlaylist::DatabaseCommand_CreatePlaylist( const source_ptr& author,
                                                                const playlist_ptr& playlist )
    : DatabaseCommandLoggable( author )
    , m_playlist( playlist )
    , m_report( false ) //this ctor used when creating locally, reporting done elsewhere
{
    qDebug() << Q_FUNC_INFO;
}


void
DatabaseCommand_CreatePlaylist::exec( DatabaseImpl* lib )
{
    createPlaylist(lib, false);
}


void
DatabaseCommand_CreatePlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;
    if( m_report == false )
        return;

    qDebug() << Q_FUNC_INFO << "..reporting..";
    if( m_playlist.isNull() ) {
        source_ptr src = source();
        QMetaObject::invokeMethod( TomahawkApp::instance()->mainWindow(),
                                "createPlaylist",
                                Qt::BlockingQueuedConnection,
                                QGenericArgument( "Tomahawk::source_ptr", (const void*)&src ),
                                Q_ARG( QVariant, m_v )
                                 );
    } else {
        m_playlist->reportCreated( m_playlist );
    }
    

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}

void 
DatabaseCommand_CreatePlaylist::createPlaylist( DatabaseImpl* lib, bool dynamic)
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !( m_playlist.isNull() && m_v.isNull() ) );
    Q_ASSERT( !source().isNull() );
    
    TomahawkSqlQuery cre = lib->newquery();
    cre.prepare( "INSERT INTO playlist( guid, source, shared, title, info, creator, lastmodified, dynplaylist) "
                 "VALUES( :guid, :source, :shared, :title, :info, :creator, :lastmodified, :dynplaylist )" );
    
    cre.bindValue( ":source", source()->isLocal() ? QVariant(QVariant::Int) : source()->id() );
    cre.bindValue( ":dynplaylist", dynamic );
    if( !m_playlist.isNull() ) {
        cre.bindValue( ":guid", m_playlist->guid() );
        cre.bindValue( ":shared", m_playlist->shared() );
        cre.bindValue( ":title", m_playlist->title() );
        cre.bindValue( ":info", m_playlist->info() );
        cre.bindValue( ":creator", m_playlist->creator() );
        cre.bindValue( ":lastmodified", m_playlist->lastmodified() );
    } else {
        QVariantMap m = m_v.toMap();
        cre.bindValue( ":guid", m.value( "guid" ) );
        cre.bindValue( ":shared", m.value( "shared" ) );
        cre.bindValue( ":title", m.value( "title" ) );
        cre.bindValue( ":info", m.value( "info" ) );
        cre.bindValue( ":creator", m.value( "creator" ) );
        cre.bindValue( ":lastmodified", m.value( "lastmodified", 0 ) );
    }
    qDebug() << "CREATE PLAYLIST:" << cre.boundValues();
    
    cre.exec();
}
