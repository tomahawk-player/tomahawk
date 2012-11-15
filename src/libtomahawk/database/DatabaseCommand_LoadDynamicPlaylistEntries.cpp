/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "DatabaseCommand_LoadDynamicPlaylistEntries.h"

#include "DatabaseImpl.h"
#include "TomahawkSqlQuery.h"
#include "playlist/dynamic/DynamicControl.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/dynamic/GeneratorFactory.h"
#include "qjson/parser.h"
#include "utils/Logger.h"
#include "Source.h"

#include <QSqlQuery>
#include <QString>

using namespace Tomahawk;


void
DatabaseCommand_LoadDynamicPlaylistEntries::exec( DatabaseImpl* dbi )
{
//    qDebug() << "Loading dynamic playlist guid" << guid();
    // load the entries first
    generateEntries( dbi );

    // now load the controls etc

    TomahawkSqlQuery controlsQuery = dbi->newquery();
    controlsQuery.prepare("SELECT playlist_revision.playlist, controls, plmode, pltype "
                          "FROM dynamic_playlist_revision, playlist_revision "
                          "WHERE dynamic_playlist_revision.guid = ? AND playlist_revision.guid = dynamic_playlist_revision.guid");
    controlsQuery.addBindValue( revisionGuid() );
    controlsQuery.exec();

    QString type;
    GeneratorMode mode;

    QList< QVariantMap > controls;
    QString playlist_guid;
//    qDebug() << "Loading controls..." << revisionGuid();
//    qDebug() << "SELECT playlist_revision.playlist, controls, plmode, pltype "
//    "FROM dynamic_playlist_revision, playlist_revision "
//    "WHERE dynamic_playlist_revision.guid = "<< revisionGuid() << " AND playlist_revision.guid = dynamic_playlist_revision.guid";

    if( controlsQuery.first() )
    {
        playlist_guid = controlsQuery.value( 0 ).toString();
        QJson::Parser parser;
        bool ok;
        QVariant v = parser.parse( controlsQuery.value(1).toByteArray(), &ok );
        Q_ASSERT( ok && v.type() == QVariant::List ); //TODO


        type = controlsQuery.value( 3 ).toString();
        mode = static_cast<GeneratorMode>( controlsQuery.value( 2 ).toInt() );

        QStringList controlIds = v.toStringList();
//        qDebug() << "Got controls in dynamic playlist, loading:" << controlIds << controlsQuery.value(1);
        foreach( const QString& controlId, controlIds )
        {
            TomahawkSqlQuery controlQuery = dbi->newquery();
            controlQuery.prepare( "SELECT selectedType, match, input "
                                  "FROM dynamic_playlist_controls "
                                  "WHERE id = :id" );
            controlQuery.bindValue( ":id", controlId );
            controlQuery.exec();
            if( controlQuery.next() )
            {
                QVariantMap c;
                c[ "type" ] = type;
                c[ "id" ] = controlId;
                c[ "selectedType" ] = controlQuery.value( 0 ).toString();
                c[ "match" ] = controlQuery.value( 1 ).toString();
                c[ "input" ] = controlQuery.value( 2 ).toString();
                controls << c;
            }
        }
    }
    else
    {
        // No controls or plguid is null, but that's okay. We'll get a setdynrevision command with a proper revision some point later
        return;
    }

    if( mode == OnDemand )
    {
//        Q_ASSERT( m_entrymap.isEmpty() ); // ondemand should have no entry

        emit done( revisionGuid(), m_islatest, type, controls, true );
    }
    else
    {
        emit done( revisionGuid(), m_guids, m_oldentries, type, controls, m_islatest, m_entrymap, true );
    }
}
