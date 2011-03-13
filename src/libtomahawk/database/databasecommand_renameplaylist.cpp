#include "databasecommand_renameplaylist.h"

#include <QSqlQuery>

#include "network/servent.h"

using namespace Tomahawk;


DatabaseCommand_RenamePlaylist::DatabaseCommand_RenamePlaylist( const source_ptr& source, const QString& playlistguid, const QString& playlistTitle )
    : DatabaseCommandLoggable( source )
{
    setPlaylistguid( playlistguid );
    setPlaylistTitle( playlistTitle );
}


void
DatabaseCommand_RenamePlaylist::exec( DatabaseImpl* lib )
{
    qDebug() << Q_FUNC_INFO;

    TomahawkSqlQuery cre = lib->newquery();

    QString sql = QString( "UPDATE playlist SET title = :title WHERE guid = :id AND source %1" )
                     .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) );

    cre.prepare( sql );
    cre.bindValue( ":id", m_playlistguid );
    cre.bindValue( ":title", m_playlistTitle );

    qDebug() << Q_FUNC_INFO << m_playlistTitle << m_playlistguid;

    cre.exec();
}


void
DatabaseCommand_RenamePlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO << "..reporting..";
    if ( source().isNull() || source()->collection().isNull() )
    {
        qDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }
    
    playlist_ptr playlist = source()->collection()->playlist( m_playlistguid );
    Q_ASSERT( !playlist.isNull() );

    qDebug() << "Renaming old playlist" << playlist->title() << "to" << m_playlistTitle << m_playlistguid;
    playlist->setTitle( m_playlistTitle );

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
