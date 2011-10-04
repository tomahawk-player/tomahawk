/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "collection.h"

#include <QMetaObject>
#include <QGenericArgument>

#include "source.h"

#include "utils/logger.h"

using namespace Tomahawk;


Collection::Collection( const source_ptr& source, const QString& name, QObject* parent )
    : QObject( parent )
    , m_name( name )
    , m_lastmodified( 0 )
    , m_isLoaded( false )
    , m_source( source )
{
    qDebug() << Q_FUNC_INFO << name << source->friendlyName();
}


Collection::~Collection()
{
    qDebug() << Q_FUNC_INFO;
}


QString
Collection::name() const
{
    return m_name;
}


const
source_ptr& Collection::source() const
{
    return m_source;
}


void
Collection::addPlaylist( const Tomahawk::playlist_ptr& p )
{
    if ( m_playlists.contains( p->guid() ) )
        return;

    QList<playlist_ptr> toadd;
    toadd << p;
//    qDebug() << "Inserted playlist with guid:" << p->guid();
    m_playlists.insert( p->guid(), p );

/*    qDebug() << Q_FUNC_INFO << "Collection name" << name()
                            << "from source id" << source()->id()
                            << "numplaylists:" << m_playlists.count();*/
    emit playlistsAdded( toadd );
}


void
Collection::addAutoPlaylist( const Tomahawk::dynplaylist_ptr& p )
{
    QList<dynplaylist_ptr> toadd;
    toadd << p;
//    qDebug() << "Inserted dynamic playlist with guid:" << p->guid();
    m_autoplaylists.insert( p->guid(), p );

/*    qDebug() << Q_FUNC_INFO << "Collection name" << name()
                            << "from source id" << source()->id()
                            << "numplaylists:" << m_playlists.count();*/
    emit autoPlaylistsAdded( toadd );
}


void
Collection::addStation( const dynplaylist_ptr& s )
{
    QList<dynplaylist_ptr> toadd;
    toadd << s;
//    qDebug() << "Inserted station with guid:" << s->guid();
    m_stations.insert( s->guid(), s );

/*    qDebug() << Q_FUNC_INFO << "Collection name" << name()
                            << "from source id" << source()->id()
                            << "numplaylists:" << m_playlists.count();*/
    emit stationsAdded( toadd );
}


void
Collection::deletePlaylist( const Tomahawk::playlist_ptr& p )
{
    QList<playlist_ptr> todelete;
    todelete << p;
    m_playlists.remove( p->guid() );

/*    qDebug() << Q_FUNC_INFO << "Collection name" << name()
                            << "from source id" << source()->id()
                            << "numplaylists:" << m_playlists.count();*/
    emit playlistsDeleted( todelete );
}


void
Collection::deleteAutoPlaylist( const Tomahawk::dynplaylist_ptr& p )
{
    QList<dynplaylist_ptr> todelete;
    todelete << p;
    m_autoplaylists.remove( p->guid() );

/*    qDebug() << Q_FUNC_INFO << "Collection name" << name()
                            << "from source id" << source()->id()
                            << "numplaylists:" << m_playlists.count();*/
    emit autoPlaylistsDeleted( todelete );
}


void
Collection::deleteStation( const dynplaylist_ptr& s )
{
    QList<dynplaylist_ptr> todelete;
    todelete << s;
    m_stations.remove( s->guid() );

/*    qDebug() << Q_FUNC_INFO << "Collection name" << name()
                            << "from source id" << source()->id()
                            << "numplaylists:" << m_playlists.count();*/
    emit stationsDeleted( todelete );
}


playlist_ptr
Collection::playlist( const QString& guid )
{
    return m_playlists.value( guid, playlist_ptr() );
}


dynplaylist_ptr
Collection::autoPlaylist( const QString& guid )
{
    return m_autoplaylists.value( guid, dynplaylist_ptr() );
}


dynplaylist_ptr
Collection::station( const QString& guid )
{

    return m_stations.value( guid, dynplaylist_ptr() );
}

QList< query_ptr >
Collection::tracks()
{
    return m_tracks;
}


void
Collection::setPlaylists( const QList<Tomahawk::playlist_ptr>& plists )
{
//    qDebug() << Q_FUNC_INFO << plists.count();
    foreach ( const playlist_ptr& p, plists )
    {
//        qDebug() << "Batch inserting playlist:" << p->guid();
        m_playlists.insert( p->guid(), p );
    }
    emit playlistsAdded( plists );
}


void
Collection::setAutoPlaylists( const QList< Tomahawk::dynplaylist_ptr >& plists )
{
//    qDebug() << Q_FUNC_INFO << plists.count();

    foreach ( const dynplaylist_ptr& p, plists )
    {
//        qDebug() << "Batch inserting dynamic playlist:" << p->guid();
        m_autoplaylists.insert( p->guid(), p );
    }
    emit autoPlaylistsAdded( plists );
}


void
Collection::setStations( const QList< dynplaylist_ptr >& stations )
{
//    qDebug() << Q_FUNC_INFO << stations.count();

    foreach ( const dynplaylist_ptr& s, stations )
    {
//        qDebug() << "Batch inserting station:" << s->guid();
        m_stations.insert( s->guid(), s );
    }
    emit autoPlaylistsAdded( stations );
}


void
Collection::setTracks( const QList<Tomahawk::query_ptr>& tracks )
{
//    qDebug() << Q_FUNC_INFO << tracks.count() << name();

    m_tracks << tracks;
    emit tracksAdded( tracks );
}


void
Collection::delTracks( const QStringList& files )
{
    qDebug() << Q_FUNC_INFO << files.count() << name();

    QList<Tomahawk::query_ptr> tracks;
    int i = 0;
    foreach ( const query_ptr& query, m_tracks )
    {
        foreach ( const QString& file, files )
        {
            bool found = false;
            foreach ( const result_ptr& result, query->results() )
            {
                if ( file == result->url() )
                {
//                    qDebug() << Q_FUNC_INFO << "Found deleted result:" << file;
                    tracks << query;
                    m_tracks.removeAt( i );
                    i--;
                    found = true;
                    break;
                }
            }
            if ( found )
                break;
        }

        i++;
    }

    tDebug() << "Emitting tracks removed:" << tracks.size();
    emit tracksRemoved( tracks );
}


void
Collection::moveAutoToStation( const QString& guid )
{
    if( m_autoplaylists.contains( guid ) )
        m_stations.insert( guid, m_autoplaylists.take( guid ) );
}


void
Collection::moveStationToAuto( const QString& guid )
{
    if( m_stations.contains( guid ) )
        m_autoplaylists.insert( guid, m_stations.take( guid ) );
}
