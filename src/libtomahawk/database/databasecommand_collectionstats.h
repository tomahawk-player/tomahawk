#ifndef DATABASECOMMAND_COLLECTIONSTATS_H
#define DATABASECOMMAND_COLLECTIONSTATS_H

#include <QVariantMap>

#include "databasecommand.h"
#include "source.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_CollectionStats : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_CollectionStats( const Tomahawk::source_ptr& source, QObject* parent = 0 );
    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "collectionstats"; }

signals:
    void done( const QVariantMap& );
};

#endif // DATABASECOMMAND_COLLECTIONSTATS_H
