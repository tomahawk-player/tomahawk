#ifndef DATABASECOMMAND_ALLTRACKS_H
#define DATABASECOMMAND_ALLTRACKS_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "tomahawk/collection.h"
#include "tomahawk/typedefs.h"

class DatabaseCommand_AllTracks : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_AllTracks( const Tomahawk::collection_ptr& collection, QObject* parent = 0 )
        : DatabaseCommand( parent ), m_collection( collection )
    {}

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "alltracks"; }

signals:
    void tracks( const QList<QVariant>&, const Tomahawk::collection_ptr& );
    void done( const Tomahawk::collection_ptr& );

private:
    Tomahawk::collection_ptr m_collection;
};

#endif // DATABASECOMMAND_ALLTRACKS_H
