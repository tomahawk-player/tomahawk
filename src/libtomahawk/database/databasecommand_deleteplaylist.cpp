#include "databasecommand_deleteplaylist.h"

#include <QSqlQuery>

#include "network/servent.h"

using namespace Tomahawk;


DatabaseCommand_DeletePlaylist::DatabaseCommand_DeletePlaylist( const source_ptr& source, const QString& playlistguid )
    : DatabaseCommandLoggable( source )
{
    setPlaylistguid( playlistguid );
}


void
DatabaseCommand_DeletePlaylist::exec( DatabaseImpl* lib )
{
    qDebug() << Q_FUNC_INFO;

    TomahawkSqlQuery cre = lib->newquery();

    QString sql = QString( "DELETE FROM playlist WHERE guid = :id AND source %1" )
                  .arg( source()->isLocal() ? "IS NULL" : QString("= %1").arg( source()->id() ) );
    cre.prepare( sql );
    cre.bindValue( ":id", m_playlistguid );

    cre.exec();
}


void
DatabaseCommand_DeletePlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO << "..reporting..";
    if ( source().isNull() || source()->collection().isNull() )
    {
        qDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }
    
    playlist_ptr playlist = source()->collection()->playlist( m_playlistguid );
    Q_ASSERT( !playlist.isNull() );

    playlist->reportDeleted( playlist );

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
