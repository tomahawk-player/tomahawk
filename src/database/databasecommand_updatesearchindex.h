#ifndef DATABASECOMMAND_UPDATESEARCHINDEX_H
#define DATABASECOMMAND_UPDATESEARCHINDEX_H

#include "databasecommand.h"
#include "databaseimpl.h"

class DatabaseCommand_UpdateSearchIndex : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_UpdateSearchIndex(const QString& table, int pkey);

    virtual QString commandname() const { return "updatesearchindex"; }
    virtual bool doesMutates() const { return true; }
    virtual void exec(DatabaseImpl* db);

signals:
    void indexUpdated();

public slots:

private:
    QString table;
    int pkey;

};

#endif // DATABASECOMMAND_UPDATESEARCHINDEX_H
