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

#include "utils/Logger.h"


namespace Tomahawk
{

DatabaseCommand::DatabaseCommand( QObject* parent )
    : QObject( parent )
    , m_state( PENDING )
{
    //qDebug() << Q_FUNC_INFO;
}


DatabaseCommand::DatabaseCommand( const Tomahawk::source_ptr& src, QObject* parent )
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
    emitRunning();
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

}
