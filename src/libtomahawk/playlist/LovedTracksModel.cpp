/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "LovedTracksModel_p.h"

#include "SourceList.h"


void
LovedTracksModel::init()
{
    Q_D( LovedTracksModel );
    d->smoothingTimer.setInterval( 300 );
    d->smoothingTimer.setSingleShot( true );

    connect( &d->smoothingTimer, SIGNAL( timeout() ), this, SLOT( loadTracks() ) );
}


LovedTracksModel::LovedTracksModel( QObject *parent )
    : PlaylistModel( parent, new LovedTracksModelPrivate( this ) )
{
    init();
}


LovedTracksModel::LovedTracksModel(QObject *parent, LovedTracksModelPrivate *d)
    : PlaylistModel( parent, d )
{
    init();
}


LovedTracksModel::~LovedTracksModel()
{
}


unsigned int
LovedTracksModel::limit() const
{
    Q_D( const LovedTracksModel );
    return d->limit;
}


void
LovedTracksModel::setLimit( unsigned int limit )
{
    Q_D( LovedTracksModel );
    d->limit = limit;
}


bool
LovedTracksModel::isTemporary() const
{
    return true;
}

void
LovedTracksModel::loadTracks()
{
    // Implement this in subclasses.
}

void
LovedTracksModel::onSourcesReady()
{
    Q_D( LovedTracksModel );
    Q_ASSERT( d->source.isNull() );

    loadTracks();

    foreach ( const Tomahawk::source_ptr& source, SourceList::instance()->sources() )
        onSourceAdded( source );
}


void
LovedTracksModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source.data(), SIGNAL( socialAttributesChanged( QString ) ), SLOT( onTrackLoved() ), Qt::UniqueConnection );
}


void
LovedTracksModel::onTrackLoved()
{
    Q_D( LovedTracksModel );
    d->smoothingTimer.start();
}


void
LovedTracksModel::tracksLoaded( QList< Tomahawk::query_ptr > newLoved )
{
    finishLoading();

    QList< Tomahawk::query_ptr > tracks;

    foreach ( const Tomahawk::plentry_ptr ple, playlistEntries() )
        tracks << ple->query();

    bool changed = false;
    QList< Tomahawk::query_ptr > mergedTracks = TomahawkUtils::mergePlaylistChanges( tracks, newLoved, changed );

    if ( changed )
    {
        QList<Tomahawk::plentry_ptr> el = playlist()->entriesFromQueries( mergedTracks, true );

        clear();
        appendEntries( el );
    }
}


void
LovedTracksModel::setSource( const Tomahawk::source_ptr& source )
{
    Q_D( LovedTracksModel );
    d->source = source;
    if ( source.isNull() )
    {
        if ( SourceList::instance()->isReady() )
            onSourcesReady();
        else
            connect( SourceList::instance(), SIGNAL( ready() ), SLOT( onSourcesReady() ) );

        connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    }
    else
    {
        onSourceAdded( source );
        loadTracks();
    }
}
