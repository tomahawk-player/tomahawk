/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "LovedTracksModel.h"

#include <QMimeData>
#include <QTreeView>

#include "Source.h"
#include "SourceList.h"
#include "database/Database.h"
#include "database/DatabaseCommand_GenericSelect.h"
#include "PlayableItem.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#define LOVED_TRACK_ITEMS 25

using namespace Tomahawk;


LovedTracksModel::LovedTracksModel( QObject* parent )
    : PlaylistModel( parent )
    , m_limit( LOVED_TRACK_ITEMS )
{
}


LovedTracksModel::~LovedTracksModel()
{
}


void
LovedTracksModel::loadTracks()
{
    if ( rowCount( QModelIndex() ) )
    {
        clear();
    }
    startLoading();

    QString sql;
    if ( m_source.isNull() )
    {
        sql = QString( "SELECT track.name, artist.name, source, COUNT(*) as counter "
                       "FROM social_attributes, track, artist "
                       "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' AND social_attributes.v = 'true' "
                       "GROUP BY track.id "
                       "ORDER BY counter DESC, social_attributes.timestamp DESC LIMIT 0, 50" );
    }
    else
    {
        sql = QString( "SELECT track.name, artist.name, COUNT(*) as counter "
                       "FROM social_attributes, track, artist "
                       "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' AND social_attributes.v = 'true' AND social_attributes.source %1 "
                       "GROUP BY track.id "
                       "ORDER BY counter DESC, social_attributes.timestamp DESC " ).arg( m_source->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_source->id() ) );
    }

    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, -1, 0 );
    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( appendQueries( QList<Tomahawk::query_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
LovedTracksModel::onSourcesReady()
{
    Q_ASSERT( m_source.isNull() );

    loadTracks();

    foreach ( const source_ptr& source, SourceList::instance()->sources() )
        onSourceAdded( source );
}


void
LovedTracksModel::setSource( const Tomahawk::source_ptr& source )
{
    m_source = source;
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


void
LovedTracksModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source.data(), SIGNAL( socialAttributesChanged( QString ) ), SLOT( onTrackLoved() ), Qt::UniqueConnection );
}


void
LovedTracksModel::onTrackLoved()
{
    loadTracks();
}


bool
LovedTracksModel::isTemporary() const
{
    return true;
}
