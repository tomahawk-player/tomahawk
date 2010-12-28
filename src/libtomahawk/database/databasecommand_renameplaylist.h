#ifndef DATABASECOMMAND_RENAMEPLAYLIST_H
#define DATABASECOMMAND_RENAMEPLAYLIST_H

#include "databaseimpl.h"
#include "databasecommandloggable.h"
#include "source.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_RenamePlaylist : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QString playlistguid READ playlistguid WRITE setPlaylistguid )
Q_PROPERTY( QString playlistTitle READ playlistTitle WRITE setPlaylistTitle )

public:
    explicit DatabaseCommand_RenamePlaylist( QObject* parent = 0 )
            : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_RenamePlaylist( const Tomahawk::source_ptr& source, const QString& playlistguid, const QString& playlistTitle );

    QString commandname() const { return "renameplaylist"; }

    virtual void exec( DatabaseImpl* lib );
    virtual void postCommitHook();
    virtual bool doesMutates() const { return true; }

    QString playlistguid() const { return m_playlistguid; }
    void setPlaylistguid( const QString& s ) { m_playlistguid = s; }

    QString playlistTitle() const { return m_playlistTitle; }
    void setPlaylistTitle( const QString& s ) { m_playlistTitle = s; }

private:
    QString m_playlistguid;
    QString m_playlistTitle;
};

#endif // DATABASECOMMAND_RENAMEPLAYLIST_H
