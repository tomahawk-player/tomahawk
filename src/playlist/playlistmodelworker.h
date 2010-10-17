#ifndef PLAYLISTMODELWORKER_H
#define PLAYLISTMODELWORKER_H

#include <QThread>

#include "tomahawk/collection.h"
#include "tomahawk/typedefs.h"

class PlaylistItem;
class PlaylistModel;

class PlaylistModelWorker : public QThread
{
Q_OBJECT

public:
    explicit PlaylistModelWorker( const QList<Tomahawk::query_ptr>& q, PlaylistModel* m, Tomahawk::collection_ptr c )
        : QThread(), m_queries( q ), m_model( m ), m_collection( c )
    {
        moveToThread( this );
    }

    explicit PlaylistModelWorker( const QList<Tomahawk::plentry_ptr>& e, PlaylistModel* m, Tomahawk::collection_ptr c )
        : QThread(), m_entries( e ), m_model( m ), m_collection( c )
    {
        moveToThread( this );
    }

    virtual void run();

public slots:
    void go();

    const QList<Tomahawk::query_ptr>& queries() { return m_queries; }
    const QList<Tomahawk::plentry_ptr>& entries() { return m_entries; }
    PlaylistModel* model() {return m_model; }
    Tomahawk::collection_ptr collection() { return m_collection; }

signals:
    void appendBatch( const QList<PlaylistItem*>& items, bool emitsig );

private:
    const QList<Tomahawk::query_ptr> m_queries;
    const QList<Tomahawk::plentry_ptr> m_entries;
    PlaylistModel* m_model;
    Tomahawk::collection_ptr m_collection;
};

#endif // PLAYLISTMODELWORKER_H
