#ifndef DATABASERESOLVER_H
#define DATABASERESOLVER_H

#include "tomahawk/resolver.h"
#include "tomahawk/result.h"

class DatabaseResolver : public Tomahawk::Resolver
{
Q_OBJECT

public:
    explicit DatabaseResolver( bool searchlocal, int weight );

    virtual QString name() const;
    virtual unsigned int weight() const { return m_weight; }
    virtual unsigned int preference() const { return 100; }
    virtual unsigned int timeout() const { return 1000; }

    virtual void resolve( QVariant v );

private slots:
    void gotResults( const Tomahawk::QID qid, QList< Tomahawk::result_ptr> results );

private:
    bool m_searchlocal;
    int m_weight;
};

#endif // DATABASERESOLVER_H
