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

    virtual void loadAllTracks( boost::function<void( const QList<QVariant>&, Tomahawk::collection_ptr )> callback );
    virtual void loadPlaylists();

public slots:
    virtual void addTracks( const QList<QVariant> &newitems );
    virtual void removeTracks( const QList<QVariant> &olditems );

    void callCallback( const QList<QVariant>& res );

private:
    boost::function<void( const QList<QVariant>&, Tomahawk::collection_ptr )> m_callback;
};

#endif // DATABASECOLLECTION_H
