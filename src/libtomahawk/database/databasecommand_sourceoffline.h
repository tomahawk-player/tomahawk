#ifndef DATABASECOMMAND_SOURCEOFFLINE_H
#define DATABASECOMMAND_SOURCEOFFLINE_H

#include "databasecommand.h"
#include "databaseimpl.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_SourceOffline : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_SourceOffline( int id );
    bool doesMutates() const { return true; }
    void exec( DatabaseImpl* lib );

private:
    int m_id;
};

#endif // DATABASECOMMAND_SOURCEOFFLINE_H
