#include "playlistmodelworker.h"

#include "playlistmodel.h"
#include "playlistitem.h"
#include "databasecollection.h"
#include "tomahawk/collection.h"

using namespace Tomahawk;


void
PlaylistModelWorker::run()
{
    qDebug() << Q_FUNC_INFO << thread();
    QTimer::singleShot( 0, this, SLOT( go() ) );
    exec();
}


void
PlaylistModelWorker::go()
{
    qDebug() << Q_FUNC_INFO << thread();

    using namespace Tomahawk;
    QList<PlaylistItem*> items;
    foreach( const query_ptr& query, m_queries )
    {
        if ( !m_collection.isNull() )
        {
            // FIXME: needs merging
            // Manually add a result, since it's coming from the local collection
            QVariantMap t = query->toVariant().toMap();
            t["score"] = 1.0;
            QList<result_ptr> results;
            result_ptr result = result_ptr( new Result( t, m_collection ) );
            results << result;
            query->addResults( results );
        }

        items << new PlaylistItem( query );

        if ( m_model->m_artists.contains( query->artist() ) )
            m_model->m_artists[query->artist()]++;
        else
            m_model->m_artists.insert( query->artist(), 1 );

        if ( items.length() % 100 == 0 )
        {
            emit appendBatch( items, false );
            items.clear();
        }
    }

    foreach( const plentry_ptr& entry, m_entries )
    {
        if ( !m_collection.isNull() )
        {
            // FIXME: needs merging
            // Manually add a result, since it's coming from the local collection
            QVariantMap t = entry->query()->toVariant().toMap();
            t["score"] = 1.0;
            QList<result_ptr> results;
            result_ptr result = result_ptr( new Result( t, m_collection ) );
            results << result;
            entry->query()->addResults( results );
        }

        items << new PlaylistItem( entry );

        if ( m_model->m_artists.contains( entry->query()->artist() ) )
            m_model->m_artists[entry->query()->artist()]++;
        else
            m_model->m_artists.insert( entry->query()->artist(), 1 );

        if ( items.length() % 400 == 0 )
        {
            emit appendBatch( items, false );
            items.clear();
        }
    }

    if ( items.length() > 0 )
    {
        emit appendBatch( items, false );
        items.clear();
    }

    exit( 0 );
}
