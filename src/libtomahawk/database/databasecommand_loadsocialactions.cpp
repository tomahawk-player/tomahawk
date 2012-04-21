/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christopher Reichert <creichert07@gmail.com>
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

#include "databasecommand_loadsocialactions.h"

#include <QSqlQuery>

#include "database/database.h"
#include "DatabaseImpl.h"
#include "network/Servent.h"
#include "Result.h"
#include "utils/logger.h"

using namespace Tomahawk;


void
DatabaseCommand_LoadSocialActions::exec( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    TomahawkSqlQuery query = dbi->newquery();

    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();

    int artid = dbi->artistId( m_artist, false );
    if( artid < 1 )
        return;

    int trkid = dbi->trackId( artid, m_track, false );
    if( trkid < 1 )
        return;

    QString whereToken;
    whereToken = QString( "WHERE id IS %1" ).arg( trkid );

    QString sql = QString(
            "SELECT k, v, timestamp, source "
            "FROM social_attributes %1 "
            "ORDER BY timestamp ASC" ).arg( whereToken );

    query.prepare( sql );
    query.exec();

    QList< Tomahawk::SocialAction > allSocialActions;
    while ( query.next() )
    {
        Tomahawk::SocialAction action;
        action.action    = query.value( 0 );  // action
        action.value     = query.value( 1 );  // comment
        action.timestamp = query.value( 2 );  // timestamp
        action.source    = SourceList::instance()->get( query.value( 3 ).toInt() );  // source
        
        if ( !action.source.isNull() )
            allSocialActions.append( action );
    }

    m_query->setAllSocialActions( allSocialActions );
}

