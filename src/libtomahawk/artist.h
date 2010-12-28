#ifndef TOMAHAWKARTIST_H
#define TOMAHAWKARTIST_H

#include <QObject>
#include <QSharedPointer>

#include "typedefs.h"
#include "collection.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT Artist : public QObject
{
Q_OBJECT

public:
    static artist_ptr get( unsigned int id, const QString& name, const Tomahawk::collection_ptr& collection );

    Artist( unsigned int id, const QString& name, const Tomahawk::collection_ptr& collection );

    unsigned int id() const { return m_id; }
    QString name() const { return m_name; }

    Tomahawk::collection_ptr collection() const { return m_collection; }

//    QList<Tomahawk::query_ptr> tracks();
//    virtual int trackCount() const { return 0; }

private:
    unsigned int m_id;
    QString m_name;

    Tomahawk::collection_ptr m_collection;
};

}; // ns

#endif
