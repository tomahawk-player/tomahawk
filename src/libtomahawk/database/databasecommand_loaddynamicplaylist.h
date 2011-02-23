#ifndef DATABASECOMMAND_LOADDYNAMICPLAYLIST_H
#define DATABASECOMMAND_LOADDYNAMICPLAYLIST_H

#include <QObject>
#include <QVariantMap>

#include "typedefs.h"
#include "databasecommand.h"
#include "databasecommand_loadplaylistentries.h"
#include "playlist.h"
#include "dynamic/DynamicControl.h"

class DatabaseCommand_LoadDynamicPlaylist : public DatabaseCommand_LoadPlaylistEntries
{
    Q_OBJECT
    
public:
    explicit DatabaseCommand_LoadDynamicPlaylist( QString revision_guid, QObject* parent = 0 )
    : DatabaseCommand_LoadPlaylistEntries( revision_guid, parent )
    { qDebug() << "loaded with:" << revision_guid << guid(); }
    
    
    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loaddynamicplaylist"; }
    
signals:
    // used if loading an ondemand playlist
    void done( QString,
                     bool,
                     QString,
                     QList< QVariantMap>,
                     bool );
    // used when loading a static playlist
    void done( QString,
               QList< QString >,
               QList< QString >,
               QString,
               QList< QVariantMap>,
               bool,
               QMap< QString, Tomahawk::plentry_ptr >,
               bool );
    
private:
};

#endif // DATABASECOMMAND_LOADDYNAMICPLAYLIST_H
