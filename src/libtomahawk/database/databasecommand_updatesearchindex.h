#ifndef DATABASECOMMAND_UPDATESEARCHINDEX_H
#define DATABASECOMMAND_UPDATESEARCHINDEX_H

#include "databasecommand.h"
#include "databaseimpl.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_UpdateSearchIndex : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_UpdateSearchIndex();

    virtual QString commandname() const { return "updatesearchindex"; }
    virtual bool doesMutates() const { return true; }
    virtual void exec( DatabaseImpl* db );

signals:
    void indexUpdated();

private:
    void indexTable( DatabaseImpl* db, const QString& table );
    
    QString table;
};

#endif // DATABASECOMMAND_UPDATESEARCHINDEX_H
