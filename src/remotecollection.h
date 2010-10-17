#ifndef REMOTECOLLECTION_H
#define REMOTECOLLECTION_H

#include "tomahawk/typedefs.h"

#include "controlconnection.h"
#include "databasecollection.h"

class RemoteCollection : public DatabaseCollection
{
Q_OBJECT

friend class ControlConnection; // for receiveTracks()

public:
    explicit RemoteCollection( Tomahawk::source_ptr source, QObject* parent = 0 );
    ~RemoteCollection()
    {
        qDebug() << Q_FUNC_INFO;
    }

public slots:
    virtual void addTracks( const QList<QVariant> &newitems );
    virtual void removeTracks( const QList<QVariant> &olditems );
};

#endif // REMOTECOLLECTION_H
