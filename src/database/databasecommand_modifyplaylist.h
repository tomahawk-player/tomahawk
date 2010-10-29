#ifndef DATABASECOMMAND_MODIFYPLAYLIST_H
#define DATABASECOMMAND_MODIFYPLAYLIST_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "tomahawk/source.h"
#include "tomahawk/playlist.h"

class DatabaseCommand_ModifyPlaylist : public DatabaseCommand
{
Q_OBJECT
Q_PROPERTY( int mode READ mode WRITE setMode )

public:
    enum Mode
    {
        ADD = 1,
        REMOVE = 2,
        UPDATE = 3
    };

    explicit DatabaseCommand_ModifyPlaylist( Tomahawk::Playlist* playlist, const QList< Tomahawk::plentry_ptr >& entries, Mode mode );

    virtual bool doesMutates() const { return true; }

    virtual void exec( DatabaseImpl* lib );

    int mode() const { return m_mode; }
    void setMode( int m ) { m_mode = (Mode)m; }

private:
    Tomahawk::Playlist* m_playlist;
    QList< Tomahawk::plentry_ptr > m_entries;
    Mode m_mode;
};

#endif // DATABASECOMMAND_MODIFYPLAYLIST_H
