#ifndef DATABASECOMMANDLOGGABLE_H
#define DATABASECOMMANDLOGGABLE_H

#include "database/databasecommand.h"

#include "dllmacro.h"

/// A Database Command that will be added to the oplog and sent over the network
/// so peers can sync up and changes to our collection in their cached copy.
class DLLEXPORT DatabaseCommandLoggable : public DatabaseCommand
{
Q_OBJECT
Q_PROPERTY(QString command READ commandname)

public:

    explicit DatabaseCommandLoggable( QObject* parent = 0 )
        : DatabaseCommand( parent )
    {}

    explicit DatabaseCommandLoggable( const Tomahawk::source_ptr& s, QObject* parent = 0 )
        : DatabaseCommand( s, parent )
    {}

    virtual bool loggable() const { return true; }

    static DatabaseCommandLoggable* factory( const QVariantMap& c );

};

#endif // DATABASECOMMANDLOGGABLE_H
