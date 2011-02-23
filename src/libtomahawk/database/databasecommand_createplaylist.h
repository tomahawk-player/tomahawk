#ifndef DATABASECOMMAND_CREATEPLAYLIST_H
#define DATABASECOMMAND_CREATEPLAYLIST_H

#include "databaseimpl.h"
#include "databasecommandloggable.h"
#include "playlist.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_CreatePlaylist : public DatabaseCommandLoggable
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
        if( m_v.isNull() )
            return QJson::QObjectHelper::qobject2qvariant( (QObject*)m_playlist.data() );
        else
            return m_v;
    }

    void setPlaylistV( const QVariant& v )
    {
        m_v = v;
    }
    
protected:
    void createPlaylist( DatabaseImpl* lib, bool dynamic = false );
    
    bool report() { return m_report; }
    void setPlaylist( const Tomahawk::playlist_ptr& playlist ) { m_playlist = playlist; }
    
    QVariant m_v;
private:
    Tomahawk::playlist_ptr m_playlist;
    bool m_report; // call Playlist::reportCreated?
};

#endif // DATABASECOMMAND_CREATEPLAYLIST_H
