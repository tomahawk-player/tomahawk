#ifndef DATABASECOMMAND_CREATEPLAYLIST_H
#define DATABASECOMMAND_CREATEPLAYLIST_H

#include "databaseimpl.h"
#include "databasecommandloggable.h"
#include "tomahawk/playlist.h"
#include "tomahawk/typedefs.h"

class DatabaseCommand_CreatePlaylist : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QVariant playlist READ playlistV WRITE setPlaylistV )

public:
    explicit DatabaseCommand_CreatePlaylist( QObject* parent = 0 );
    explicit DatabaseCommand_CreatePlaylist( const Tomahawk::source_ptr& author, const Tomahawk::playlist_ptr& playlist );

    QString commandname() const { return "createplaylist"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }

    QVariant playlistV() const
    {
        return QJson::QObjectHelper::qobject2qvariant( (QObject*)m_playlist.data() );
    }

    void setPlaylistV( const QVariant& v )
    {
        qDebug() << "***********" << Q_FUNC_INFO << v;
        using namespace Tomahawk;

        Playlist* p = new Playlist( source() );
        QJson::QObjectHelper::qvariant2qobject( v.toMap(), p );
        m_playlist = playlist_ptr( p );
    }

private:
    Tomahawk::playlist_ptr m_playlist;
    bool m_report; // call Playlist::reportCreated?
};

#endif // DATABASECOMMAND_CREATEPLAYLIST_H
