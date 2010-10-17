#ifndef DATABASECOMMAND_IMPORTALLPLAYLIST_H
#define DATABASECOMMAND_IMPORTALLPLAYLIST_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "tomahawk/typedefs.h"

class DatabaseCommand_LoadAllPlaylists : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_LoadAllPlaylists( const Tomahawk::source_ptr& s, QObject* parent = 0 )
        : DatabaseCommand( s, parent )
    {}

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadallplaylists"; }

signals:
    void done( const QList<Tomahawk::playlist_ptr>& playlists );
};

#endif // DATABASECOMMAND_ADDFILES_H
