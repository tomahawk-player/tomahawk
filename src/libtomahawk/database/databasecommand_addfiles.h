#ifndef DATABASECOMMAND_ADDFILES_H
#define DATABASECOMMAND_ADDFILES_H

#include <QObject>
#include <QVariantMap>

#include "database/databasecommandloggable.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_AddFiles : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QVariantList files READ files WRITE setFiles )

public:
    explicit DatabaseCommand_AddFiles( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_AddFiles( const QList<QVariant>& files, const Tomahawk::source_ptr& source, QObject* parent = 0 )
        : DatabaseCommandLoggable( parent ), m_files( files )
    {
        setSource( source );
    }

    virtual QString commandname() const { return "addfiles"; }

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return true; }
    virtual void postCommitHook();

    QVariantList files() const;
    void setFiles( const QVariantList& f ) { m_files = f; }

signals:
    void done( const QList<QVariant>&, Tomahawk::collection_ptr );
    void notify( const QList<QVariant>&, Tomahawk::collection_ptr );

private:
    QVariantList m_files;
};

#endif // DATABASECOMMAND_ADDFILES_H
