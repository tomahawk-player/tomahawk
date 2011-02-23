#ifndef DATABASECOMMAND_LOADALLSOURCES_H
#define DATABASECOMMAND_LOADALLSOURCES_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_LoadAllSources : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_LoadAllSources( QObject* parent = 0 )
        : DatabaseCommand( parent )
    {}

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadallsources"; }

signals:
    void done( const QList<Tomahawk::source_ptr>& sources );
};

#endif // DATABASECOMMAND_LOADALLSOURCES_H
