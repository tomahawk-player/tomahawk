#include "tomahawk/collection.h"

#include <QMetaObject>
#include <QGenericArgument>

#include "tomahawk/playlist.h"

using namespace Tomahawk;


Collection::Collection( const source_ptr& source, const QString& name, QObject* parent )
    : QObject( parent )
    , m_name( name )
    , m_loaded( false )
    , m_lastmodified( 0 )
    , m_source( source )
{
//    qDebug() << Q_FUNC_INFO;
}


Collection::~Collection()
{
    qDebug() << Q_FUNC_INFO;
}


void
Collection::invokeSlotTracks( QObject* obj, const char* slotname,
                              const QList<QVariant>& val,
                              collection_ptr collection )
{
    qDebug() << Q_FUNC_INFO << obj << slotname;
    QMetaObject::invokeMethod( obj, slotname, Qt::QueuedConnection,
                               Q_ARG( QList<QVariant>, val ),
                               Q_ARG( Tomahawk::collection_ptr, collection ) );
}


QString
Collection::name() const
{
    return m_name;
}


void
Collection::addPlaylist( const Tomahawk::playlist_ptr& p )
{
    qDebug() << Q_FUNC_INFO;
    QList<playlist_ptr> toadd;
    toadd << p;
    m_playlists.append( toadd );

    qDebug() << Q_FUNC_INFO << "Collection name" << name()
                            << "from source id" << source()->id()
                            << "numplaylists:" << m_playlists.length();
    emit playlistsAdded( toadd );
}


void
Collection::deletePlaylist( const Tomahawk::playlist_ptr& p )
{
    qDebug() << Q_FUNC_INFO;
    QList<playlist_ptr> todelete;
    todelete << p;
    m_playlists.removeAll( p );

    qDebug() << Q_FUNC_INFO << "Collection name" << name()
                            << "from source id" << source()->id()
                            << "numplaylists:" << m_playlists.length();
    emit playlistsDeleted( todelete );
}


void
Collection::loadTracks( QObject* obj, const char* slotname )
{
    if ( !obj )
        obj = this;

    boost::function< void( const QList<QVariant>&, Tomahawk::collection_ptr )> cb =
            boost::bind( &Collection::invokeSlotTracks, this, obj, slotname, _1, _2 );
    loadAllTracks( cb );
}


playlist_ptr
Collection::playlist( const QString& guid )
{
    foreach( const playlist_ptr& pp, m_playlists )
    {
        if( pp->guid() == guid )
            return pp;
    }
    
    return playlist_ptr();
}


bool
Collection::trackSorter( const QVariant& left, const QVariant& right )
{
    int art = left.toMap().value( "artist" ).toString()
              .localeAwareCompare( right.toMap().value( "artist" ).toString() );

    if ( art == 0 )
    {
        int trk = left.toMap().value( "track" ).toString()
                  .localeAwareCompare( right.toMap().value( "track" ).toString() );
        return trk < 0;
    }
    else
    {
        return art < 0;
    }
}
