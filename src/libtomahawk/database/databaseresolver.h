#ifndef DATABASERESOLVER_H
#define DATABASERESOLVER_H

#include "pipeline.h"

#include "resolver.h"
#include "result.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseResolver : public Tomahawk::Resolver
{
Q_OBJECT

public:
    explicit DatabaseResolver( int weight );

    virtual QString name() const;
    virtual unsigned int weight() const { return m_weight; }
    virtual unsigned int preference() const { return 100; }
    virtual unsigned int timeout() const { return 2500; }

public slots:
    virtual void resolve( const Tomahawk::query_ptr& query );

private slots:
    void gotResults( const Tomahawk::QID qid, QList< Tomahawk::result_ptr> results );

private:
    int m_weight;
};

#endif // DATABASERESOLVER_H
