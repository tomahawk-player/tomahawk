#ifndef DATABASECOMMAND_DELETEFILES_H
#define DATABASECOMMAND_DELETEFILES_H

#include <QObject>
#include <QDir>
#include <QVariantMap>

#include "database/databasecommandloggable.h"
#include "typedefs.h"
#include "query.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_DeleteFiles : public DatabaseCommandLoggable
{
Q_OBJECT
Q_PROPERTY( QVariantList ids READ ids WRITE setIds )

public:
    explicit DatabaseCommand_DeleteFiles( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
    {}

    explicit DatabaseCommand_DeleteFiles( const QDir& dir, const Tomahawk::source_ptr& source, QObject* parent = 0 )
    : DatabaseCommandLoggable( parent ), m_dir( dir )
    {
        setSource( source );
    }
    
    virtual QString commandname() const { return "deletefiles"; }

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return true; }
    virtual void postCommitHook();

    QStringList files() const { return m_files; }
    void setFiles( const QStringList& f ) { m_files = f; }

    QVariantList ids() const { return m_ids; }
    void setIds( const QVariantList& i ) { m_ids = i; }

signals:
    void done( const QStringList&, const Tomahawk::collection_ptr& );
    void notify( const QStringList&, const Tomahawk::collection_ptr& );

private:
    QDir m_dir;
    QStringList m_files;
    QVariantList m_ids;
};

#endif // DATABASECOMMAND_DELETEFILES_H
