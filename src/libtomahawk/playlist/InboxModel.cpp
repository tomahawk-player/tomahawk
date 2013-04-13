/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "InboxModel.h"

#include "database/Database.h"
#include "database/DatabaseCommand_GenericSelect.h"
#include "SourceList.h"
#include "utils/Logger.h"
#include "utils/Closure.h"


InboxModel::InboxModel( QObject* parent )
    : PlaylistModel( parent )
{
    if ( SourceList::instance()->isReady() )
        loadTracks();
    else
        NewClosure( SourceList::instance(), SIGNAL( ready() ),
                    this, SLOT( loadTracks() ) );
}


InboxModel::~InboxModel()
{}


QList<Tomahawk::SocialAction>
InboxModel::mergeSocialActions( QList<Tomahawk::SocialAction> first, QList<Tomahawk::SocialAction> second)
{
    foreach ( Tomahawk::SocialAction sa, second )
    {
        if ( sa.action != "Inbox" )
        {
            first.append( sa );
            continue;
        }

        bool contains = false;
        for ( int i = 0; i < first.count(); ++i )
        {
            Tomahawk::SocialAction &sb = first[ i ];
            if ( sa.source == sb.source )
            {
                sb.timestamp = qMax( sa.timestamp.toInt(), sb.timestamp.toInt() );
                sb.value = sa.value.toBool() && sb.value.toBool();
                contains = true;
                break;
            }
        }
        if ( !contains )
            first.append( sa );
    }
    return first;
}


void
InboxModel::insertEntries( const QList< Tomahawk::plentry_ptr >& entries, int row )
{
    QList< Tomahawk::plentry_ptr > toInsert;
    for ( QList< Tomahawk::plentry_ptr >::const_iterator it = entries.constBegin();
          it != entries.constEnd(); ++it )
    {
        Tomahawk::plentry_ptr entry = *it;
        bool gotDupe = false;
        for ( QList< Tomahawk::plentry_ptr >::iterator jt = toInsert.begin();
              jt != toInsert.end(); ++jt  )
        {
            Tomahawk::plentry_ptr existingEntry = *jt;
            if ( entry->query()->equals( existingEntry->query(), true /*ignoreCase*/) )
            {
                //We got a dupe, let's merge the social actions
                existingEntry->query()->setAllSocialActions( mergeSocialActions( existingEntry->query()->allSocialActions(),
                                                                                 entry->query()->allSocialActions() ) );
                gotDupe = true;
                break;
            }
        }
        if ( !gotDupe )
            toInsert.append( entry );
    }

    foreach ( Tomahawk::plentry_ptr plEntry, playlistEntries() )
    {
        for ( int i = 0; i < toInsert.count(); )
        {
            if ( plEntry->query()->equals( toInsert.at( i )->query(), true ) )
            {
                plEntry->query()->setAllSocialActions( mergeSocialActions( plEntry->query()->allSocialActions(),
                                                                           toInsert.at( i )->query()->allSocialActions() ) );
                toInsert.removeAt( i );

                dataChanged( index( playlistEntries().indexOf( plEntry ), 0, QModelIndex() ),
                             index( playlistEntries().indexOf( plEntry ), columnCount() -1, QModelIndex() ) );
            }
            else
                ++i;
        }
    }
    changed();

    PlaylistModel::insertEntries( toInsert, row );
}


void
InboxModel::clear()
{
    PlaylistModel::clear();
}


void
InboxModel::loadTracks()
{
    startLoading();

    //extra fields end up in Tomahawk query objects as qt properties
    QString sql = QString( "SELECT track.name as title, artist.name as artist, source, v as unlistened, social_attributes.timestamp "
                           "FROM social_attributes, track, artist "
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Inbox' "
                           "ORDER BY social_attributes.timestamp DESC" );

    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, -1, 0 );
    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksLoaded( QList<Tomahawk::query_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
InboxModel::tracksLoaded( QList< Tomahawk::query_ptr > newTracks )
{
    finishLoading();

    QList< Tomahawk::query_ptr > tracks;

    foreach ( const Tomahawk::plentry_ptr ple, playlistEntries() )
        tracks << ple->query();

    foreach ( Tomahawk::query_ptr newQuery, newTracks )
    {
        QVariantList extraData = newQuery->property( "data" ).toList();

        Tomahawk::SocialAction action;
        action.action = "Inbox";
        action.source = SourceList::instance()->get( extraData.at( 0 ).toInt() );
        action.value = extraData.at( 1 ).toBool(); //unlistened
        action.timestamp = extraData.at( 2 ).toUInt();

        QList< Tomahawk::SocialAction > actions;
        actions << action;
        newQuery->setAllSocialActions( actions );

        newQuery->setProperty( "data", QVariant() ); //clear
    }

    bool changed = false;
    QList< Tomahawk::query_ptr > mergedTracks = TomahawkUtils::mergePlaylistChanges( tracks, newTracks, changed );

    if ( changed )
    {
        QList< Tomahawk::plentry_ptr > el = playlist()->entriesFromQueries( mergedTracks, true );

        clear();
        appendEntries( el );
    }
}

