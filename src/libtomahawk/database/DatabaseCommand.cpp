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

#include "DatabaseCommand.h"

#include "DatabaseCommand_AddFiles.h"
#include "DatabaseCommand_CreatePlaylist.h"
#include "DatabaseCommand_DeleteFiles.h"
#include "DatabaseCommand_DeletePlaylist.h"
#include "DatabaseCommand_LogPlayback.h"
#include "DatabaseCommand_RenamePlaylist.h"
#include "DatabaseCommand_SetPlaylistRevision.h"
#include "DatabaseCommand_CreateDynamicPlaylist.h"
#include "DatabaseCommand_DeleteDynamicPlaylist.h"
#include "DatabaseCommand_SetDynamicPlaylistRevision.h"
#include "DatabaseCommand_SocialAction.h"
#include "DatabaseCommand_ShareTrack.h"
#include "DatabaseCommand_SetCollectionAttributes.h"
#include "DatabaseCommand_SetTrackAttributes.h"

#include "utils/Logger.h"
#include "utils/Uuid.h"

#include <boost/function.hpp>

DatabaseCommand::DatabaseCommand( QObject* parent )
    : QObject( parent )
    , m_state( PENDING )
{
    //qDebug() << Q_FUNC_INFO;
}


DatabaseCommand::DatabaseCommand( const source_ptr& src, QObject* parent )
    : QObject( parent )
    , m_state( PENDING )
    , m_source( src )
{
    //qDebug() << Q_FUNC_INFO;
}

DatabaseCommand::DatabaseCommand( const DatabaseCommand& other )
    : QObject( other.parent() )
{
}

DatabaseCommand::~DatabaseCommand()
{
//    qDebug() << Q_FUNC_INFO;
}


void
DatabaseCommand::_exec( DatabaseImpl* lib )
{
    //qDebug() << "RUNNING" << thread();
    m_state = RUNNING;
    emit running();
    exec( lib );
    m_state = FINISHED;
    //qDebug() << "FINISHED" << thread();
}


void
DatabaseCommand::setSource( const Tomahawk::source_ptr& s )
{
    m_source = s;
}


const Tomahawk::source_ptr&
DatabaseCommand::source() const
{
    return m_source;
}


DatabaseCommand*
DatabaseCommand::factory( const QVariant& op, const source_ptr& source )
{
    const QString name = op.toMap().value( "command" ).toString();

    if( name == "addfiles" )
    {
        DatabaseCommand_AddFiles * cmd = new DatabaseCommand_AddFiles;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "deletefiles" )
    {
        DatabaseCommand_DeleteFiles * cmd = new DatabaseCommand_DeleteFiles;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "createplaylist" )
    {
        DatabaseCommand_CreatePlaylist * cmd = new DatabaseCommand_CreatePlaylist;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "deleteplaylist" )
    {
        DatabaseCommand_DeletePlaylist * cmd = new DatabaseCommand_DeletePlaylist;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "logplayback" )
    {
        DatabaseCommand_LogPlayback * cmd = new DatabaseCommand_LogPlayback;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "renameplaylist" )
    {
        DatabaseCommand_RenamePlaylist * cmd = new DatabaseCommand_RenamePlaylist;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "setplaylistrevision" )
    {
        DatabaseCommand_SetPlaylistRevision * cmd = new DatabaseCommand_SetPlaylistRevision;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "createdynamicplaylist" )
    {
        DatabaseCommand_CreateDynamicPlaylist * cmd = new DatabaseCommand_CreateDynamicPlaylist;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "deletedynamicplaylist" )
    {
        DatabaseCommand_DeleteDynamicPlaylist * cmd = new DatabaseCommand_DeleteDynamicPlaylist;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "setdynamicplaylistrevision" )
    {
        qDebug() << "SETDYN CONTENT:" << op;
        DatabaseCommand_SetDynamicPlaylistRevision * cmd = new DatabaseCommand_SetDynamicPlaylistRevision;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "socialaction" )
    {
        DatabaseCommand_SocialAction * cmd = new DatabaseCommand_SocialAction;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "setcollectionattributes" )
    {
        DatabaseCommand_SetCollectionAttributes * cmd = new DatabaseCommand_SetCollectionAttributes;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "settrackattributes" )
    {
        DatabaseCommand_SetTrackAttributes * cmd = new DatabaseCommand_SetTrackAttributes;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }
    else if( name == "sharetrack" )
    {
        DatabaseCommand_ShareTrack * cmd = new DatabaseCommand_ShareTrack;
        cmd->setSource( source );
        QJson::QObjectHelper::qvariant2qobject( op.toMap(), cmd );
        return cmd;
    }

    qDebug() << "Unknown database command" << name;
//    Q_ASSERT( false );
    return NULL;
}

QString DatabaseCommand::guid() const
{
    if( m_guid.isEmpty() )
        m_guid = ::uuid();

    return m_guid;
}
