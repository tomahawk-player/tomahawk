/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "DatabaseCommand_CreatePlaylist.h"

#include <QSqlQuery>

#include "SourceList.h"
#include "DatabaseImpl.h"
#include "TomahawkSqlQuery.h"
#include "network/Servent.h"
#include "utils/Logger.h"

using namespace Tomahawk;


DatabaseCommand_CreatePlaylist::DatabaseCommand_CreatePlaylist( QObject* parent )
    : DatabaseCommandLoggable( parent )
    , m_report( true )
{
}


DatabaseCommand_CreatePlaylist::DatabaseCommand_CreatePlaylist( const source_ptr& author,
                                                                const playlist_ptr& playlist )
    : DatabaseCommandLoggable( author )
    , m_playlist( playlist )
    , m_report( false ) //this ctor used when creating locally, reporting done elsewhere
{
}

DatabaseCommand_CreatePlaylist::~DatabaseCommand_CreatePlaylist()
{}

void
DatabaseCommand_CreatePlaylist::exec( DatabaseImpl* lib )
{
    createPlaylist( lib, false );
}

QVariant
DatabaseCommand_CreatePlaylist::playlistV() const
{
    if( m_v.isNull() )
        return QJson::QObjectHelper::qobject2qvariant( (QObject*)m_playlist.data() );
    else
        return m_v;
 }


void
DatabaseCommand_CreatePlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;

    if ( source()->isLocal() )
        Servent::instance()->triggerDBSync();

    if ( m_report == false )
        return;

    tDebug() << Q_FUNC_INFO << "reporting...";
    if ( m_playlist.isNull() )
    {
        QMetaObject::invokeMethod( SourceList::instance(),
                                   "createPlaylist",
                                   Qt::BlockingQueuedConnection,
                                   QGenericArgument( "Tomahawk::source_ptr", (const void*)&source() ),
                                   Q_ARG( QVariant, m_v ) );
    }
    else
    {
        m_playlist->reportCreated( m_playlist );
    }
}


void
DatabaseCommand_CreatePlaylist::createPlaylist( DatabaseImpl* lib, bool dynamic)
{
    Q_ASSERT( !( m_playlist.isNull() && m_v.isNull() ) );
    Q_ASSERT( !source().isNull() );

    uint now = 0;
    if ( m_playlist.isNull() )
    {
        now = m_v.toMap()[ "createdon" ].toUInt();
    }
    else
    {
        now = QDateTime::currentDateTime().toTime_t();
        m_playlist->setCreatedOn( now );
    }

    TomahawkSqlQuery cre = lib->newquery();
    cre.prepare( "INSERT INTO playlist( guid, source, shared, title, info, creator, lastmodified, dynplaylist, createdOn ) "
                 "VALUES( :guid, :source, :shared, :title, :info, :creator, :lastmodified, :dynplaylist, :createdOn )" );

    cre.bindValue( ":source", source()->isLocal() ? QVariant(QVariant::Int) : source()->id() );
    cre.bindValue( ":dynplaylist", dynamic );
    cre.bindValue( ":createdOn", now );
    if ( !m_playlist.isNull() )
    {
        cre.bindValue( ":guid", m_playlist->guid() );
        cre.bindValue( ":shared", m_playlist->shared() );
        cre.bindValue( ":title", m_playlist->title() );
        cre.bindValue( ":info", m_playlist->info() );
        cre.bindValue( ":creator", m_playlist->creator() );
        cre.bindValue( ":lastmodified", m_playlist->lastmodified() );
    }
    else
    {
        QVariantMap m = m_v.toMap();
        cre.bindValue( ":guid", m.value( "guid" ) );
        cre.bindValue( ":shared", m.value( "shared" ) );
        cre.bindValue( ":title", m.value( "title" ) );
        cre.bindValue( ":info", m.value( "info" ) );
        cre.bindValue( ":creator", m.value( "creator" ) );
        cre.bindValue( ":lastmodified", m.value( "lastmodified", 0 ) );
    }

    cre.exec();
}
