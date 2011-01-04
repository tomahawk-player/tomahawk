#ifndef DATABASECOMMAND_H
#define DATABASECOMMAND_H

#include <QObject>
#include <QMetaType>
#include <QTime>
#include <QSqlQuery>

#include "source.h"
#include "typedefs.h"
#include "database/op.h"

#include "dllmacro.h"

class DatabaseImpl;

class DLLEXPORT DatabaseCommand : public QObject
{
Q_OBJECT
Q_PROPERTY( QString guid READ guid WRITE setGuid )

public:
    enum State {
        PENDING = 0,
        RUNNING = 1,
        FINISHED = 2
    };

    explicit DatabaseCommand( QObject* parent = 0 );
    explicit DatabaseCommand( const Tomahawk::source_ptr& src, QObject* parent = 0 );

    DatabaseCommand( const DatabaseCommand &other )
    {
    }

    virtual ~DatabaseCommand();

    virtual QString commandname() const { return "DatabaseCommand"; }
    virtual bool doesMutates() const { return true; }
    State state() const { return m_state; }

    // if i make this pure virtual, i get compile errors in qmetatype.h.
    // we need Q_DECLARE_METATYPE to use in queued sig/slot connections.
    virtual void exec( DatabaseImpl* lib ) { Q_ASSERT( false ); }

    void _exec( DatabaseImpl* lib );

    // stuff to do once transaction applied ok.
    // Don't change the database from in here, duh.
    void postCommit() { postCommitHook(); emit committed(); }
    virtual void postCommitHook(){};

    void setSource( const Tomahawk::source_ptr& s ) { m_source = s; }
    const Tomahawk::source_ptr& source() const { return m_source; }

    virtual bool loggable() const { return false; }

    QString guid() const
    {
        if( m_guid.isEmpty() )
            m_guid = uuid();

        return m_guid;
    }
    void setGuid( const QString& g ) { m_guid = g; }

    void emitFinished() { emit finished(); }

    static DatabaseCommand* factory( const QVariant& op, const Tomahawk::source_ptr& source );

signals:
    void running();
    void finished();
    void committed();

private:
    State m_state;
    Tomahawk::source_ptr m_source;
    mutable QString m_guid;
};

Q_DECLARE_METATYPE( DatabaseCommand )

#endif // DATABASECOMMAND_H
