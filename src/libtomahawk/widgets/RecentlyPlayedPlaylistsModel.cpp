/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
    Copyright (C) 2011  Jeff Mitchell <jeff@tomahawk-player.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "RecentlyPlayedPlaylistsModel.h"

#include "TomahawkSettings.h"
#include "audio/AudioEngine.h"
#include "SourceList.h"
#include "utils/Logger.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include "Playlist.h"

using namespace Tomahawk;


RecentlyPlayedPlaylistsModel::RecentlyPlayedPlaylistsModel( QObject* parent )
    : QAbstractListModel( parent )
    , m_maxPlaylists( 0 )
    , m_waitingForSome( true )
{
    loadFromSettings();

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), this, SLOT( onSourceAdded( Tomahawk::source_ptr ) ), Qt::QueuedConnection );
    connect( TomahawkSettings::instance(), SIGNAL( recentlyPlayedPlaylistAdded( QString, int ) ), this, SLOT( plAdded( QString, int ) ) );
    connect( AudioEngine::instance(),SIGNAL( playlistChanged( Tomahawk::playlistinterface_ptr ) ), this, SLOT( playlistChanged( Tomahawk::playlistinterface_ptr ) ), Qt::QueuedConnection );

    emit emptinessChanged( m_recplaylists.isEmpty() );
}


void
RecentlyPlayedPlaylistsModel::loadFromSettings()
{
//    qDebug() << Q_FUNC_INFO;
    if( !m_waitingForSome )
        return;

    beginResetModel();
    m_recplaylists.clear();
    m_waitingForSome = false;

    QStringList playlist_guids = TomahawkSettings::instance()->recentlyPlayedPlaylistGuids( m_maxPlaylists );

    for( int i = playlist_guids.size() - 1; i >= 0; i-- )
    {
//        qDebug() << "loading playlist" << playlist_guids[i];

        playlist_ptr pl = m_cached.value( playlist_guids[i], Tomahawk::playlist_ptr() );
        if( pl.isNull() )
            pl = Tomahawk::Playlist::load( playlist_guids[i] );
        if( pl.isNull() )
            pl = Tomahawk::DynamicPlaylist::load( playlist_guids[i] );

        if ( !pl.isNull() ) {
            m_recplaylists << pl;

            if( !m_cached.contains( playlist_guids[i] ) )
            {
                if ( pl.dynamicCast< DynamicPlaylist >().isNull() )
                    connect( pl.data(), SIGNAL(revisionLoaded(Tomahawk::PlaylistRevision)), this, SLOT(playlistRevisionLoaded()) );
                else
                    connect( pl.data(), SIGNAL(dynamicRevisionLoaded(Tomahawk::DynamicPlaylistRevision)), this, SLOT(playlistRevisionLoaded()) );
                m_cached[playlist_guids[i]] = pl;
            }
        } else
            m_waitingForSome = true;
    }
    endResetModel();

    emit emptinessChanged( m_recplaylists.isEmpty() );
}


QVariant
RecentlyPlayedPlaylistsModel::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() || !hasIndex( index.row(), index.column(), index.parent() ) )
        return QVariant();

    playlist_ptr pl = m_recplaylists[index.row()];
    switch( role )
    {
    case Qt::DisplayRole:
        return pl->title();
    case PlaylistRole:
        return QVariant::fromValue< Tomahawk::playlist_ptr >( pl );
    case ArtistRole:
    {
        if( m_artists.value( pl ).isEmpty() )
        {
            QStringList artists;

            foreach( const Tomahawk::plentry_ptr& entry, pl->entries() )
            {
                if ( !artists.contains( entry->query()->artist() ) )
                    artists << entry->query()->artist();
            }

            m_artists[pl] = artists.join( ", " );
        }

        return m_artists[pl];
    }
    case PlaylistTypeRole:
    {
        if ( !pl.dynamicCast< Tomahawk::DynamicPlaylist >().isNull() )
        {
            dynplaylist_ptr dynp = pl.dynamicCast< Tomahawk::DynamicPlaylist >();
            if ( dynp->mode() == Static )
                return AutoPlaylist;
            else if ( dynp->mode() == OnDemand )
                return Station;
        } else
        {
            return StaticPlaylist;
        }
    }
    case DynamicPlaylistRole:
    {
        dynplaylist_ptr dynp = pl.dynamicCast< Tomahawk::DynamicPlaylist >();
        return QVariant::fromValue< Tomahawk::dynplaylist_ptr >( dynp );
    }
    case TrackCountRole:
    {
        if ( !pl.dynamicCast< Tomahawk::DynamicPlaylist >().isNull() && pl.dynamicCast< Tomahawk::DynamicPlaylist >()->mode() == OnDemand )
            return QString( QChar( 0x221E ) );
        else
            return pl->entries().count();
    }
    default:
        return QVariant();
    }
}

void
RecentlyPlayedPlaylistsModel::playlistRevisionLoaded()
{
    Playlist* p = qobject_cast< Playlist* >( sender() );
    Q_ASSERT( p );

    for ( int i = 0; i < m_recplaylists.size(); i++ )
    {
        if ( m_recplaylists[ i ]->guid() == p->guid() )
        {
            QModelIndex idx = index( i, 0, QModelIndex() );
            emit dataChanged( idx, idx );
        }
    }
}


void
RecentlyPlayedPlaylistsModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source.data(), SIGNAL( online() ), this, SLOT( sourceOnline() ) );
    connect( source->dbCollection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ), SLOT( loadFromSettings() ) );
    connect( source->dbCollection().data(), SIGNAL( playlistsDeleted( QList<Tomahawk::playlist_ptr> ) ), SLOT( onPlaylistsRemoved( QList<Tomahawk::playlist_ptr> ) ) );
}

void
RecentlyPlayedPlaylistsModel::sourceOnline()
{
    Source* s = qobject_cast< Source* >( sender() );
    Q_ASSERT( s );

    for ( int i = 0; i < m_recplaylists.size(); i++ )
    {
        if ( m_recplaylists[ i ]->author().data() == s )
        {
            QModelIndex idx = index( i, 0, QModelIndex() );
            emit dataChanged( idx, idx );
        }
    }
}


void
RecentlyPlayedPlaylistsModel::onPlaylistsRemoved( QList< playlist_ptr > playlists )
{
    foreach( const playlist_ptr& pl, playlists ) {
        if( m_recplaylists.contains( pl ) ) {
            m_artists.remove( pl );
            m_cached.remove( pl->guid() );

            int idx = m_recplaylists.indexOf( pl );
            beginRemoveRows( QModelIndex(), idx, idx );
            m_recplaylists.removeAt( idx );
            endRemoveRows();
        }
    }

    emit emptinessChanged( m_recplaylists.isEmpty() );
}


int
RecentlyPlayedPlaylistsModel::rowCount( const QModelIndex& ) const
{
    return m_recplaylists.count();
}


void
RecentlyPlayedPlaylistsModel::plAdded( const QString& plguid, int sId )
{
    source_ptr source = SourceList::instance()->get( sId );
    if ( source.isNull() )
        return;

    playlist_ptr pl = source->dbCollection()->playlist( plguid );
    if ( pl.isNull() )
        pl = source->dbCollection()->autoPlaylist( plguid );
    if ( pl.isNull() )
        pl = source->dbCollection()->station( plguid );

    if ( pl.isNull() )
        return;

    onPlaylistsRemoved( QList< playlist_ptr >() << pl );

    beginInsertRows( QModelIndex(), 0, 0 );
    m_recplaylists.prepend( pl );;
    endInsertRows();

    emit emptinessChanged( m_recplaylists.isEmpty() );
}


void
RecentlyPlayedPlaylistsModel::playlistChanged( Tomahawk::playlistinterface_ptr pli )
{
    // ARG
    if ( pli.isNull() )
        return;

    if ( Playlist *pl = dynamic_cast< Playlist* >( pli.data() ) ) {
        // look for it, qsharedpointer fail
        playlist_ptr ptr;
        foreach( const playlist_ptr& test, m_recplaylists ) {
            if( test.data() == pl )
                ptr = test;
        }

        if( !ptr.isNull() && m_artists.contains( ptr ) ) {
            m_artists[ ptr ] = QString();
        }

        QModelIndex idx = index( m_recplaylists.indexOf( ptr ), 0, QModelIndex() );
        emit dataChanged( idx, idx );
    }
}
