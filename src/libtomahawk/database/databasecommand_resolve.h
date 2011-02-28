#ifndef DATABASECOMMAND_RESOLVE_H
#define DATABASECOMMAND_RESOLVE_H

#include "databasecommand.h"
#include "databaseimpl.h"
#include "result.h"

#include <QVariant>

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_Resolve : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_Resolve( const Tomahawk::query_ptr& query );

    virtual QString commandname() const { return "dbresolve"; }
    virtual bool doesMutates() const { return false; }

    virtual void exec( DatabaseImpl *lib );

signals:
    void results( Tomahawk::QID qid, QList<Tomahawk::result_ptr> results );

public slots:

private:
    Tomahawk::query_ptr m_query;

    float how_similar( const Tomahawk::query_ptr& q, const Tomahawk::result_ptr& r );
    static int levenshtein( const QString& source, const QString& target );
};

#endif // DATABASECOMMAND_RESOLVE_H
