/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

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


#include "welcomeplaylistmodel.h"
#include <tomahawksettings.h>
#include <audio/audioengine.h>
#include <sourcelist.h>

using namespace Tomahawk;

WelcomePlaylistModel::WelcomePlaylistModel( QObject* parent )
    : QAbstractListModel( parent )
{
    loadFromSettings();

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( loadFromSettings() ), Qt::QueuedConnection );
    connect( TomahawkSettings::instance(), SIGNAL( recentlyPlayedPlaylistAdded( Tomahawk::playlist_ptr ) ), this, SLOT( plAdded( Tomahawk::playlist_ptr ) ) );
    connect( AudioEngine::instance(),SIGNAL( playlistChanged( PlaylistInterface* ) ), this, SLOT( playlistChanged( PlaylistInterface* ) ), Qt::QueuedConnection );

    emit emptinessChanged( m_recplaylists.isEmpty() );
}

void
WelcomePlaylistModel::loadFromSettings()
{
    beginResetModel();
    m_recplaylists.clear();

    QStringList playlist_guids = TomahawkSettings::instance()->recentlyPlayedPlaylistGuids();

    for( int i = playlist_guids.size() - 1; i >= 0; i-- )
    {
        qDebug() << "loading playlist" << playlist_guids[i];
        Tomahawk::playlist_ptr pl = Tomahawk::Playlist::load( playlist_guids[i] );
        if ( !pl.isNull() )
            m_recplaylists << pl;
    }
    endResetModel();

    emit emptinessChanged( m_recplaylists.isEmpty() );
}


QVariant
WelcomePlaylistModel::data( const QModelIndex& index, int role ) const
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
    case TrackCountRole:
        return pl->entries().count();
    default:
        return QVariant();
    }
}

void
WelcomePlaylistModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source->collection().data(), SIGNAL( playlistsDeleted( QList<Tomahawk::playlist_ptr> ) ), SLOT( onPlaylistsRemoved( QList<Tomahawk::playlist_ptr> ) ) );
}

void
WelcomePlaylistModel::onPlaylistsRemoved( QList< playlist_ptr > playlists )
{
    foreach( const playlist_ptr& pl, playlists ) {
        if( m_recplaylists.contains( pl ) ) {
            m_artists.remove( pl );
            int idx = m_recplaylists.indexOf( pl );
            beginRemoveRows( QModelIndex(), idx, idx );
            m_recplaylists.removeAt( idx );
            endRemoveRows();
        }
    }

    emit emptinessChanged( m_recplaylists.isEmpty() );
}


int
WelcomePlaylistModel::rowCount( const QModelIndex& ) const
{
    return m_recplaylists.count();
}

void
WelcomePlaylistModel::plAdded( const playlist_ptr& pl )
{
    onPlaylistsRemoved( QList< playlist_ptr >() << pl );

    beginInsertRows( QModelIndex(), 0, 0 );
    m_recplaylists.prepend( pl );;
    endInsertRows();

    emit emptinessChanged( m_recplaylists.isEmpty() );
}

void
WelcomePlaylistModel::playlistChanged( PlaylistInterface* pli )
{
    // ARG
    if( Playlist* pl = dynamic_cast< Playlist* >( pli ) ) {
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
