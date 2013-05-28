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
#include "database/DatabaseCommand_DeleteInboxEntry.h"
#include "database/DatabaseCommand_ModifyInboxEntry.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "utils/Logger.h"
#include "utils/Closure.h"
#include "jobview/JobStatusModel.h"


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


int
InboxModel::unlistenedCount() const
{
    int count = 0;
    foreach ( const Tomahawk::plentry_ptr& plentry, playlistEntries() )
    {
        bool isUnlistened = true;
        foreach ( Tomahawk::SocialAction sa, plentry->query()->queryTrack()->allSocialActions() )
        {
            if ( sa.action == "Inbox" && sa.value.toBool() == false )
            {
                isUnlistened = false;
                break;
            }
        }
        if ( isUnlistened )
            count++;
    }
    return count;
}


void
InboxModel::insertEntries( const QList< Tomahawk::plentry_ptr >& entries, int row, const QList< Tomahawk::PlaybackLog >& logs )
{
    Q_UNUSED( logs ); // <- this is merely to silence GCC

    QList< Tomahawk::plentry_ptr > toInsert;
    for ( QList< Tomahawk::plentry_ptr >::const_iterator it = entries.constBegin();
          it != entries.constEnd(); ++it )
    {
        Tomahawk::plentry_ptr entry = *it;
        for ( QList< Tomahawk::plentry_ptr >::iterator jt = toInsert.begin();
              jt != toInsert.end(); ++jt  )
        {
            Tomahawk::plentry_ptr existingEntry = *jt;
            if ( entry->query()->equals( existingEntry->query(), true /*ignoreCase*/) )
            {
                toInsert.erase( jt );
                break;
            }
        }
        toInsert.append( entry );
    }

    foreach ( Tomahawk::plentry_ptr plEntry, playlistEntries() )
    {
        for ( int i = 0; i < toInsert.count(); )
        {
            if ( plEntry->query()->equals( toInsert.at( i )->query(), true ) )
            {
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
InboxModel::removeIndex( const QModelIndex& index, bool moreToCome )
{
    PlayableItem* item = itemFromIndex( index );
    if ( item && !item->query().isNull() )
    {
        DatabaseCommand_DeleteInboxEntry* cmd = new DatabaseCommand_DeleteInboxEntry( item->query() );
        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
    }

    PlaylistModel::removeIndex( index, moreToCome );
}


void
InboxModel::clear()
{
    PlaylistModel::clear();
}


void
InboxModel::showNotification( InboxJobItem::Side side,
                              const Tomahawk::source_ptr& src,
                              const Tomahawk::trackdata_ptr& track )
{
    JobStatusView::instance()->model()->addJob( new InboxJobItem( side,
                                                                  src->friendlyName(),
                                                                  track ) );

    if ( side == InboxJobItem::Receiving )
    {
        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["title"] = track->track();
        trackInfo["artist"] = track->artist();

        Tomahawk::InfoSystem::InfoStringHash sourceInfo;
        sourceInfo["friendlyname"] = src->friendlyName();

        QVariantMap playInfo;
        playInfo["trackinfo"] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
        playInfo["private"] = TomahawkSettings::instance()->privateListeningMode();
        playInfo["sourceinfo"] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( sourceInfo );

        Tomahawk::InfoSystem::InfoPushData pushData ( "InboxModel", Tomahawk::InfoSystem::InfoInboxReceived, playInfo, Tomahawk::InfoSystem::PushShortUrlFlag );
        Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
    }
}


void
InboxModel::showNotification( InboxJobItem::Side side,
                              const QString& dbid,
                              const Tomahawk::trackdata_ptr& track )
{
    Tomahawk::source_ptr src = SourceList::instance()->get( dbid );
    if ( !src.isNull() )
        showNotification( side, src, track );
}


void
InboxModel::markAsListened( const QModelIndexList& indexes )
{
    foreach ( QModelIndex index, indexes )
    {
        PlayableItem* item = itemFromIndex( index );
        if ( item && !item->query().isNull() )
        {
            item->query()->queryTrack()->markAsListened();
        }
    }
}


void
InboxModel::loadTracks()
{
    startLoading();

    //extra fields end up in Tomahawk query objects as qt properties
    QString sql = QString( "SELECT track.name as title, artist.name as artist, source, v as unlistened, social_attributes.timestamp "
                           "FROM social_attributes, track, artist "
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Inbox' "
                           "ORDER BY social_attributes.timestamp" );

    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, -1, 0 );
    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksLoaded( QList<Tomahawk::query_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
InboxModel::tracksLoaded( QList< Tomahawk::query_ptr > incoming )
{
    finishLoading();

    QList< Tomahawk::query_ptr > tracks;

    foreach ( const Tomahawk::plentry_ptr ple, playlistEntries() )
        tracks << ple->query();

    //We invert the result of the SQLite query.
    //NOTE: this operation relies on the fact that SQLite always seems to return the records in
    //      their original order of insertion when the ORDER BY criterion is equal for some records
    //      (i.e. a stable sort). This assumption might not be true for other things that talk SQL,
    //      but it should work for us as long as we stick with SQLite.  -- Teo
    QList< Tomahawk::query_ptr > newTracks;
    while ( !incoming.isEmpty() )
        newTracks.append( incoming.takeLast() );

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
        newQuery->queryTrack()->loadSocialActions();

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

