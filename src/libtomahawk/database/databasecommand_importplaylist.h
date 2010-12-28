#ifndef DATABASECOMMAND_IMPORTPLAYLIST_H
#define DATABASECOMMAND_IMPORTPLAYLIST_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "tomahawk/source.h"

#include "dllmacro.h"

class Playlist;

class DLLEXPORT DatabaseCommand_ImportPlaylist : public DatabaseCommand
{
    Q_OBJECT
public:
    explicit DatabaseCommand_ImportPlaylist(Playlist * p, QObject *parent = 0)
        : DatabaseCommand(parent), m_playlist(p)
    {}

    virtual void exec(DatabaseImpl *);
    virtual bool doesMutates() const { return true; }
    virtual QString commandname() const { return "importplaylist"; }

signals:
    void done(int id);

private:
    Playlist * m_playlist;
};

#endif // DATABASECOMMAND_ADDFILES_H
