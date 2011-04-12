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

#include "source.h"

#include "collection.h"
#include "sourcelist.h"

#include "network/controlconnection.h"
#include "database/databasecommand_addsource.h"
#include "database/databasecommand_sourceoffline.h"
#include "database/databasecommand_logplayback.h"
#include "database/database.h"

using namespace Tomahawk;


Source::Source( int id, const QString& username )
    : QObject()
    , m_isLocal( false )
    , m_online( false )
    , m_username( username )
    , m_id( id )
    , m_cc( 0 )
{
    qDebug() << Q_FUNC_INFO << id << username;

    if ( id == 0 )
    {
        m_isLocal = true;
        m_online = true;
    }
}


Source::~Source()
{
    qDebug() << Q_FUNC_INFO << friendlyName();
}


void
Source::setControlConnection( ControlConnection* cc )
{
    m_cc = cc;
    if ( cc )
        connect( cc, SIGNAL( finished() ), SLOT( remove() ), Qt::QueuedConnection );
}


collection_ptr
Source::collection() const
{
    if( m_collections.length() )
        return m_collections.first();

    collection_ptr tmp;
    return tmp;
}


void
Source::setStats( const QVariantMap& m )
{
    m_stats = m;
    emit stats( m_stats );
}


void
Source::remove()
{
    qDebug() << Q_FUNC_INFO;

    setOffline();
}


QString
Source::friendlyName() const
{
    if ( m_friendlyname.isEmpty() )
        return m_username;

    //TODO: this is a terrible assumption, help me clean this up, mighty muesli!
    if ( m_friendlyname.contains( "@conference.") )
        return QString(m_friendlyname).remove( 0, m_friendlyname.lastIndexOf( "/" )+1 ).append(" via MUC");

    if ( m_friendlyname.contains( "/tomahawk" ) )
        return m_friendlyname.left( m_friendlyname.indexOf( "/tomahawk" ) );

    return m_friendlyname;
}


void
Source::addCollection( const collection_ptr& c )
{
    Q_ASSERT( m_collections.length() == 0 ); // only 1 source supported atm
    m_collections.append( c );
    emit collectionAdded( c );
}


void
Source::removeCollection( const collection_ptr& c )
{
    Q_ASSERT( m_collections.length() == 1 && m_collections.first() == c ); // only 1 source supported atm
    m_collections.removeAll( c );
    emit collectionRemoved( c );
}


void
Source::setOffline()
{
    if ( !m_online )
        return;

    m_online = false;
    emit offline();

    m_cc = 0;
    DatabaseCommand_SourceOffline* cmd = new DatabaseCommand_SourceOffline( id() );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


void
Source::setOnline()
{
    if ( m_online )
        return;
    m_online = true;

    // ensure username is in the database
    DatabaseCommand_addSource* cmd = new DatabaseCommand_addSource( m_username, m_friendlyname );
    connect( cmd, SIGNAL( done( unsigned int, QString ) ),
                    SLOT( dbLoaded( unsigned int, const QString& ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

    emit online();
}


void
Source::dbLoaded( unsigned int id, const QString& fname )
{
    qDebug() << Q_FUNC_INFO << id << fname;

    m_id = id;
    m_friendlyname = fname;

    emit syncedWithDatabase();
}


void
Source::scanningProgress( unsigned int files )
{
    m_textStatus = tr( "Scanning (%L1 tracks)" ).arg( files );
    emit stateChanged();
}


void
Source::scanningFinished( unsigned int files )
{
    Q_UNUSED( files );
    m_textStatus = QString();
    emit stateChanged();
}


void
Source::onStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info )
{
    Q_UNUSED( oldstate );
    QString msg;
    switch( newstate )
    {
        case DBSyncConnection::CHECKING:
            msg = tr( "Checking" );
            break;
        case DBSyncConnection::FETCHING:
            msg = tr( "Fetching" );
            break;
        case DBSyncConnection::PARSING:
            msg = tr( "Parsing" );
            break;
        case DBSyncConnection::SAVING:
            msg = tr( "Saving" );
            break;
        case DBSyncConnection::SYNCED:
            msg = QString();
            break;
        case DBSyncConnection::SCANNING:
            msg = tr( "Scanning (%L1 tracks)" ).arg( info );
            break;

        default:
            msg = QString();
    }

    m_textStatus = msg;
    emit stateChanged();
}


void
Source::onPlaybackStarted( const Tomahawk::query_ptr& query )
{
    qDebug() << Q_FUNC_INFO << query->toString();
    m_currentTrack = query;
    emit playbackStarted( query );
}


void
Source::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    qDebug() << Q_FUNC_INFO << query->toString();
    emit playbackFinished( query );
}
