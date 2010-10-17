#ifndef DATABASECOMMAND_DELETEPLAYLIST_H
#define DATABASECOMMAND_DELETEPLAYLIST_H

#include "databaseimpl.h"
#include "databasecommandloggable.h"
#include "tomahawk/source.h"
#include "tomahawk/typedefs.h"

class DatabaseCommand_DeletePlaylist : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QString playlistguid READ playlistguid WRITE setPlaylistguid )

public:
    explicit DatabaseCommand_DeletePlaylist( QObject* parent = 0 )
            : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_DeletePlaylist( const Tomahawk::source_ptr& source, const QString& playlistguid );

    QString commandname() const { return "deleteplaylist"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }

    QString playlistguid() const { return m_playlistguid; }
    void setPlaylistguid( const QString& s ) { m_playlistguid = s; }

private:
    QString m_playlistguid;
};

#endif // DATABASECOMMAND_DELETEPLAYLIST_H
