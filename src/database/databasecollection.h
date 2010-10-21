#ifndef DATABASECOLLECTION_H
#define DATABASECOLLECTION_H

#include "tomahawk/collection.h"
#include "tomahawk/typedefs.h"

class DatabaseCollection : public Tomahawk::Collection
{
Q_OBJECT

public:
    explicit DatabaseCollection( const Tomahawk::source_ptr& source, QObject* parent = 0 );
    ~DatabaseCollection()
    {
        qDebug() << Q_FUNC_INFO;
    }

    virtual void loadAllTracks();
    virtual void loadPlaylists();

public slots:
    virtual void addTracks( const QList<QVariant> &newitems );
    virtual void removeTracks( const QList<QVariant> &olditems );
};

#endif // DATABASECOLLECTION_H
