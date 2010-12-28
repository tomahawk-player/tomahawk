#ifndef DATABASECOMMAND_LOADOPS_H
#define DATABASECOMMAND_LOADOPS_H

#include "typedefs.h"
#include "databasecommand.h"
#include "databaseimpl.h"
#include "op.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_loadOps : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_loadOps( const Tomahawk::source_ptr& src, QString since, QObject* parent = 0 )
        : DatabaseCommand( src ), m_since( since )
    {}

    virtual void exec( DatabaseImpl* db );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadops"; }

signals:
    void done( QString lastguid, QList< dbop_ptr > ops );

private:
    QString m_since; // guid to load from
};

#endif // DATABASECOMMAND_LOADOPS_H
