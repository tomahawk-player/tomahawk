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

#include "databasecommand_socialaction.h"

#include <QSqlQuery>

#include "database/database.h"
#include "databaseimpl.h"
#include "network/servent.h"

using namespace Tomahawk;


void
DatabaseCommand_SocialAction::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;
    if ( source()->isLocal() )
    {
        Servent::instance()->triggerDBSync();
    }
}


void
DatabaseCommand_SocialAction::exec( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    TomahawkSqlQuery query = dbi->newquery();

    query.prepare( "INSERT INTO social_attributes(id, source, k, v, timestamp) "
                   "VALUES (?, ?, ?, ?, ?)" );

    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();

    bool autoCreate = true;
    int artid = dbi->artistId( m_artist, autoCreate );
    if( artid < 1 )
        return;

    autoCreate = true; // artistId overwrites autoCreate (reference)
    int trkid = dbi->trackId( artid, m_track, autoCreate );
    if( trkid < 1 )
        return;

    query.bindValue( 0, trkid );
    query.bindValue( 1, srcid );
    query.bindValue( 2, m_action );
    query.bindValue( 3, m_comment );
    query.bindValue( 4, m_timestamp );

    query.exec();
}

