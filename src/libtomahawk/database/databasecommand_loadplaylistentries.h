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
    : DatabaseCommand( parent ), m_revguid( revision_guid )
    {}

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadplaylistentries"; }

    QString revisionGuid() const { return m_revguid; }
signals:
    void done( const QString& rev,
               const QList<QString>& orderedguid,
               const QList<QString>& oldorderedguid,
               bool islatest,
               const QMap< QString, Tomahawk::plentry_ptr >& added,
               bool applied );

protected:
    void generateEntries( DatabaseImpl* dbi );
    
    QStringList m_guids;
    QMap< QString, Tomahawk::plentry_ptr > m_entrymap;
    bool m_islatest;
    QStringList m_oldentries;
    
private:
    QString m_revguid;
};

#endif
