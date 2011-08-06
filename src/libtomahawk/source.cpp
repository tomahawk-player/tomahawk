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
#include "sourceplaylistinterface.h"

#include "network/controlconnection.h"
#include "database/databasecommand_addsource.h"
#include "database/databasecommand_sourceoffline.h"
#include "database/database.h"

#include <QCoreApplication>

#include "utils/logger.h"

using namespace Tomahawk;


Source::Source( int id, const QString& username )
    : QObject()
    , m_isLocal( false )
    , m_online( false )
    , m_username( username )
    , m_id( id )
    , m_cc( 0 )
    , m_avatar( 0 )
{
    qDebug() << Q_FUNC_INFO << id << username;

    m_scrubFriendlyName = qApp->arguments().contains( "--demo" );

    if ( id == 0 )
    {
        m_isLocal = true;
        m_online = true;
    }

    m_currentTrackTimer.setInterval( 600000 ); // 10 minutes
    m_currentTrackTimer.setSingleShot( true );
    connect( &m_currentTrackTimer, SIGNAL( timeout() ), this, SLOT( trackTimerFired() ) );
}


Source::~Source()
{
    qDebug() << Q_FUNC_INFO << friendlyName();
    delete m_avatar;
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
    if ( m_friendlyname.contains( "@conference." ) )
        return QString( m_friendlyname ).remove( 0, m_friendlyname.lastIndexOf( "/" ) + 1 ).append( " via MUC" );

    if ( m_friendlyname.contains( "/" ) )
        return m_friendlyname.left( m_friendlyname.indexOf( "/" ) );

    return m_friendlyname;
}


void
Source::setAvatar( const QPixmap& avatar )
{
    //FIXME: use a proper pixmap store that's thread-safe
    delete m_avatar;
    m_avatar = new QPixmap( avatar );
}


QPixmap
Source::avatar() const
{
    //FIXME: use a proper pixmap store that's thread-safe
    if ( m_avatar )
        return QPixmap( *m_avatar );
    else
        return QPixmap();
}


void
Source::setFriendlyName( const QString& fname )
{
    m_friendlyname = fname;
    if ( m_scrubFriendlyName )
        m_friendlyname = m_friendlyname.split( "@" ).first();
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
    qDebug() << Q_FUNC_INFO << friendlyName();
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
    qDebug() << Q_FUNC_INFO << friendlyName();
    if ( m_online )
        return;

    m_online = true;
    emit online();

    // ensure username is in the database
    DatabaseCommand_addSource* cmd = new DatabaseCommand_addSource( m_username, m_friendlyname );
    connect( cmd, SIGNAL( done( unsigned int, QString ) ),
                    SLOT( dbLoaded( unsigned int, const QString& ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


void
Source::dbLoaded( unsigned int id, const QString& fname )
{
    qDebug() << Q_FUNC_INFO << id << fname;

    m_id = id;
    setFriendlyName( fname );

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


unsigned int
Source::trackCount() const
{
    return m_stats.value( "numfiles" ).toUInt();
}


Tomahawk::playlistinterface_ptr
Source::getPlaylistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        Tomahawk::source_ptr source = SourceList::instance()->get( id() );
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::SourcePlaylistInterface( source ) );
    }

    return m_playlistInterface;
}


void
Source::onPlaybackStarted( const Tomahawk::query_ptr& query )
{
    qDebug() << Q_FUNC_INFO << query->toString();
    m_currentTrack = query;
    if ( m_playlistInterface.isNull() )
        getPlaylistInterface();
    emit playbackStarted( query );
}


void
Source::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    qDebug() << Q_FUNC_INFO << query->toString();
    emit playbackFinished( query );

    m_currentTrackTimer.start();
}

void
Source::trackTimerFired()
{
    m_currentTrack.clear();

    emit stateChanged();
}

void
Source::reportSocialAttributesChanged()
{
    emit socialAttributesChanged();
}

