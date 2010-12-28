#ifndef DATABASECOMMAND_LOADPLAYLIST_H
#define DATABASECOMMAND_LOADPLAYLIST_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "playlist.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_LoadPlaylistEntries : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_LoadPlaylistEntries( QString revision_guid, QObject* parent = 0 )
        : DatabaseCommand( parent ), m_guid( revision_guid )
    {}

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadplaylistentries"; }

signals:
    void done( const QString& rev,
               const QList<QString>& orderedguid,
               const QList<QString>& oldorderedguid,
               bool islatest,
               const QMap< QString, Tomahawk::plentry_ptr >& added,
               bool applied );

private:
    QString m_guid;
};

#endif
