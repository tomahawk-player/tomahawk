#ifndef DATABASECOMMAND_ALLTRACKS_H
#define DATABASECOMMAND_ALLTRACKS_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "tomahawk/source.h"
#include "tomahawk/typedefs.h"

class DatabaseCommand_AllTracks : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_AllTracks( const Tomahawk::source_ptr& source, QObject* parent = 0 )
        : DatabaseCommand( parent ), m_source( source )
    {}

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "alltracks"; }

signals:
    void done( const QList<QVariant>& );

private:
    Tomahawk::source_ptr m_source;
};

#endif // DATABASECOMMAND_ALLTRACKS_H
