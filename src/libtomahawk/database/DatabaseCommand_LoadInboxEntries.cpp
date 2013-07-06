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


#include "DatabaseCommand_LoadInboxEntries.h"

#include "DatabaseImpl.h"
#include "Query.h"
#include "SourceList.h"
#include "TomahawkSqlQuery.h"
#include "Track.h"
#include "TrackData.h"


namespace Tomahawk
{

DatabaseCommand_LoadInboxEntries::DatabaseCommand_LoadInboxEntries( QObject* parent )
    : DatabaseCommand( parent )
{
}


void DatabaseCommand_LoadInboxEntries::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery sqlQuery = dbi->newquery();

    QString sql = QString( "SELECT track.name as title, artist.name as artist, source, v as unlistened, social_attributes.timestamp "
                           "FROM social_attributes, track, artist "
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Inbox' "
                           "ORDER BY social_attributes.timestamp" );

    sqlQuery.prepare( sql );
    sqlQuery.exec();

    QList< Tomahawk::query_ptr > queries;
    while ( sqlQuery.next() )
    {
        QString track, artist;
        track = sqlQuery.value( 0 ).toString();
        artist = sqlQuery.value( 1 ).toString();

        Tomahawk::query_ptr query = Tomahawk::Query::get( artist, track, QString() );
        if ( query.isNull() )
            continue;

        int sourceId = sqlQuery.value( 2 ).toInt();
        bool unlistened = sqlQuery.value( 3 ).toBool();
        uint timestamp = sqlQuery.value( 4 ).toUInt();

        Tomahawk::SocialAction action;
        action.action = "Inbox";
        action.source = SourceList::instance()->get( sourceId );
        action.value = unlistened;
        action.timestamp = timestamp;

        QList< Tomahawk::SocialAction > actions;
        actions << action;

        query->queryTrack()->setAllSocialActions( actions );
        queries << query;
    }

    emit tracks( queries );
}

}
