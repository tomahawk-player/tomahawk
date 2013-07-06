/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "Source_p.h"

#include "collection/Collection.h"
#include "SourceList.h"
#include "SourcePlaylistInterface.h"

#include "accounts/AccountManager.h"
#include "network/ControlConnection.h"
#include "database/DatabaseCommand_AddSource.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "database/DatabaseCommand_LoadAllSources.h"
#include "database/DatabaseCommand_SourceOffline.h"
#include "database/DatabaseCommand_UpdateSearchIndex.h"
#include "database/DatabaseImpl.h"
#include "database/Database.h"

#include <QCoreApplication>
#include <QtAlgorithms>

#include "utils/TomahawkCache.h"
#include "database/DatabaseCommand_SocialAction.h"

#ifndef ENABLE_HEADLESS
    #include "utils/TomahawkUtilsGui.h"
#endif

#include "utils/Logger.h"
#include "sip/PeerInfo.h"

using namespace Tomahawk;


Source::Source( int id, const QString& nodeId )
    : QObject()
    , d_ptr( new SourcePrivate( this, id, nodeId ) )
{
    Q_D( Source );
    d->scrubFriendlyName = qApp->arguments().contains( "--demo" );

    if ( id == 0 )
        d->isLocal = true;

    d->currentTrackTimer.setSingleShot( true );
    connect( &d->currentTrackTimer, SIGNAL( timeout() ), this, SLOT( trackTimerFired() ) );

    if ( d->isLocal )
    {
        connect( Accounts::AccountManager::instance(),
                 SIGNAL( connected( Tomahawk::Accounts::Account* ) ),
                 SLOT( setOnline() ) );
        connect( Accounts::AccountManager::instance(),
                 SIGNAL( disconnected( Tomahawk::Accounts::Account*, Tomahawk::Accounts::AccountManager::DisconnectReason ) ),
                 SLOT( handleDisconnect( Tomahawk::Accounts::Account*, Tomahawk::Accounts::AccountManager::DisconnectReason ) ) );
    }
}


Source::~Source()
{
    tDebug() << Q_FUNC_INFO << friendlyName();
    delete d_ptr;
}

bool
Source::isLocal() const
{
    Q_D( const Source );
    return d->isLocal;
}

bool
Source::isOnline() const
{
    Q_D( const Source );
    return d->online || d->isLocal;
}


bool
Source::setControlConnection( ControlConnection* cc )
{
    Q_D( Source );

    QMutexLocker locker( &d->setControlConnectionMutex );
    if ( !d->cc.isNull() && d->cc->isReady() && d->cc->isRunning() )
    {
        const QString& nodeid = Database::instance()->impl()->dbid();
        peerInfoDebug( (*cc->peerInfos().begin()) ) << Q_FUNC_INFO << "Comparing" << cc->id() << "and" << nodeid << "to detect duplicate connection, outbound:" << cc->outbound();
        // If our nodeid is "higher" than the other, we prefer inbound connection, else outbound.
        if ( ( cc->id() < nodeid && d->cc->outbound() ) || ( cc->id() > nodeid && !d->cc->outbound() ) )
        {
            // Tell the ControlConnection it is not anymore responsible for us.
            d->cc->unbindFromSource();
            // This ControlConnection is not needed anymore, get rid of it!
            // (But decouple the deletion it from the current activity)
            QMetaObject::invokeMethod( d->cc.data(), "deleteLater", Qt::QueuedConnection);
            // Use new ControlConnection
            d->cc = cc;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        d->cc = cc;
        return true;
    }
}


const QSet<peerinfo_ptr>
Source::peerInfos() const
{
    if ( controlConnection() )
    {
        return controlConnection()->peerInfos();
    }
    else if ( isLocal() )
    {
        return PeerInfo::getAllSelf().toSet();

    }
    return QSet< Tomahawk::peerinfo_ptr >();
}


collection_ptr
Source::dbCollection() const
{
    Q_D( const Source );
    if ( d->collections.length() )
    {
        foreach ( const collection_ptr& collection, d->collections )
        {
            if ( collection->backendType() == Collection::DatabaseCollectionType )
            {
                return collection; // We assume only one is a db collection. Now get off my lawn.
            }
        }
    }

    collection_ptr tmp;
    return tmp;
}

QList<collection_ptr>
Source::collections() const
{
    return d_func()->collections;
}


void
Source::setStats( const QVariantMap& m )
{
    Q_D( Source );
    d->stats = m;
    emit stats( d->stats );
    emit stateChanged();
}


QString
Source::nodeId() const
{
    return d_func()->nodeId;

}


QString
Source::friendlyName() const
{
    Q_D( const Source );

    QStringList candidateNames;
    foreach ( const peerinfo_ptr& peerInfo, peerInfos() )
    {
        if ( !peerInfo.isNull() && !peerInfo->friendlyName().isEmpty() )
        {
            candidateNames.append( peerInfo->friendlyName() );
        }
    }

    if ( !candidateNames.isEmpty() )
    {
        if ( candidateNames.count() > 1 )
            qSort( candidateNames.begin(), candidateNames.end(), &Source::friendlyNamesLessThan );

        return candidateNames.first();
    }

    if ( d->friendlyname.isEmpty() )
    {
        return dbFriendlyName();
    }

    return d->friendlyname;
}


bool
Source::friendlyNamesLessThan( const QString& first, const QString& second )
{
    //Least favored match first.
    QList< QRegExp > penalties;
    penalties.append( QRegExp( "\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}" ) ); //IPv4 address
    penalties.append( QRegExp( "([\\w-\\.\\+]+)@((?:[\\w]+\\.)+)([a-zA-Z]{2,4})" ) ); //email/jabber id

    //Most favored match first.
    QList< QRegExp > favored;
    favored.append( QRegExp( "\\b([A-Z][a-z']* ?){2,10}" ) ); //properly capitalized person's name
    favored.append( QRegExp( "[a-zA-Z ']+" ) ); //kind of person's name

    bool matchFirst = false;
    bool matchSecond = false;

    //We check if the strings match the regexps. The regexps represent friendly name patterns we do
    //*not* want (penalties) or want (favored), prioritized. If none of the strings match a regexp,
    //we go to the next regexp. If one of the strings matches, and we're matching penalties, we say
    //the other one is lessThan, i.e. comes first. If one of the string matches, and we're matching
    //favored, we say this one is lessThan, i.e. comes first. If both strings match, or if no match
    //is found for any regexp, we go to string comparison (fallback).
    while( !penalties.isEmpty() || !favored.isEmpty() )
    {
        QRegExp rx;
        bool isPenalty;
        if ( !penalties.isEmpty() )
        {
            rx = penalties.first();
            penalties.pop_front();
            isPenalty = true;
        }
        else
        {
            rx = favored.first();
            favored.pop_front();
            isPenalty = false;
        }

        matchFirst = rx.exactMatch( first );
        matchSecond = rx.exactMatch( second );

        if ( matchFirst == false && matchSecond == false )
            continue;

        if ( matchFirst == true && matchSecond == true )
            break;

        if ( matchFirst == true && matchSecond == false )
            return isPenalty ? false : true;

        if ( matchFirst == false && matchSecond == true )
            return isPenalty ? true : false;
    }

    return ( first.compare( second ) == -1 ) ? true : false;
}


#ifndef ENABLE_HEADLESS
QPixmap
Source::avatar( TomahawkUtils::ImageMode style, const QSize& size )
{
    Q_D( Source );

    foreach ( const peerinfo_ptr& peerInfo, peerInfos() )
    {
        if ( peerInfo && !peerInfo->avatar( style, size ).isNull() )
        {
            return peerInfo->avatar( style, size );
        }
    }

    if ( d->avatarLoaded )
    {
        if ( d->avatar )
            return *d->avatar;
        else
            return QPixmap();
    }

    // Try to get the avatar from the cache
    // Hint: We store the avatar for each xmpp peer using its contactId, the dbFriendlyName is a contactId of a peer
    d->avatarLoaded = true;
    QByteArray avatarBuffer = TomahawkUtils::Cache::instance()->getData( "Sources", dbFriendlyName() ).toByteArray();
    if ( !avatarBuffer.isNull() )
    {
        tDebug() << Q_FUNC_INFO << QThread::currentThread();
        QPixmap avatar;
        avatar.loadFromData( avatarBuffer );
        avatarBuffer.clear();

        d->avatar = new QPixmap( TomahawkUtils::createRoundedImage( avatar, QSize( 0, 0 ) ) );
        return *d->avatar;
    }

    return QPixmap();
}
#endif


void
Source::setFriendlyName( const QString& fname )
{
    Q_D( Source );

    if ( fname.isEmpty() )
    {
        return;
    }

    d->friendlyname = fname;
    if ( d->scrubFriendlyName )
    {
        if ( d->friendlyname.indexOf( "@" ) > 0 )
        {
            d->friendlyname = d->friendlyname.split( "@" ).first();
        }
    }
}


QString
Source::dbFriendlyName() const
{
    Q_D( const Source );

    if( d->dbFriendlyName.isEmpty() )
    {
        return nodeId();
    }

    return d->dbFriendlyName;
}


void
Source::setDbFriendlyName( const QString& dbFriendlyName )
{
    Q_D( Source );

    if ( dbFriendlyName.isEmpty() )
        return;

    d->dbFriendlyName = dbFriendlyName;
}


void
Source::addCollection( const collection_ptr& c )
{
    Q_D( Source );

    //Q_ASSERT( m_collections.length() == 0 ); // only 1 source supported atm
    d->collections.append( c );
    emit collectionAdded( c );
}


void
Source::removeCollection( const collection_ptr& c )
{
    Q_D( Source );

    //Q_ASSERT( m_collections.length() == 1 && m_collections.first() == c ); // only 1 source supported atm
    d->collections.removeAll( c );
    emit collectionRemoved( c );
}

int
Source::id() const
{
    Q_D( const Source );

    return d->id;
}

ControlConnection*
Source::controlConnection() const
{
    Q_D( const Source );

    return d->cc.data();
}

void
Source::handleDisconnect( Tomahawk::Accounts::Account*, Tomahawk::Accounts::AccountManager::DisconnectReason reason )
{
    if ( reason == Tomahawk::Accounts::AccountManager::Disabled )
        setOffline();
}


void
Source::setOffline()
{
    Q_D( Source );

    qDebug() << Q_FUNC_INFO << friendlyName();
    if ( !d->online )
        return;

    d->online = false;
    emit offline();

    if ( !isLocal() )
    {
        d->currentTrack.clear();
        emit stateChanged();

        d->cc = 0;
        DatabaseCommand_SourceOffline* cmd = new DatabaseCommand_SourceOffline( id() );
        Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
    }
}


void
Source::setOnline()
{
    Q_D( Source );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << friendlyName();
    if ( d->online )
        return;

    d->online = true;
    emit online();

    if ( !isLocal() )
    {
        // ensure username is in the database
        DatabaseCommand_addSource* cmd = new DatabaseCommand_addSource( d->nodeId, dbFriendlyName() );
        connect( cmd, SIGNAL( done( unsigned int, QString ) ),
                        SLOT( dbLoaded( unsigned int, const QString& ) ) );
        Database::instance()->enqueue( Tomahawk::dbcmd_ptr(cmd) );
    }
}


void
Source::dbLoaded( unsigned int id, const QString& fname )
{
    Q_D( Source );

    d->id = id;
    setDbFriendlyName( fname );

    emit syncedWithDatabase();
}


void
Source::scanningProgress( unsigned int files )
{
    Q_D( Source );

    if ( files )
        d->textStatus = tr( "Scanning (%L1 tracks)" ).arg( files );
    else
        d->textStatus = tr( "Scanning" );

    emit stateChanged();
}


void
Source::scanningFinished( bool updateGUI )
{
    Q_D( Source );

    d->textStatus = QString();

    if ( d->updateIndexWhenSynced )
    {
        d->updateIndexWhenSynced = false;
        updateTracks();
    }

    emit stateChanged();

    if ( updateGUI )
        emit synced();
}


void
Source::onStateChanged( Tomahawk::DBSyncConnectionState newstate, Tomahawk::DBSyncConnectionState oldstate, const QString& info )
{
    Q_D( Source );

    Q_UNUSED( oldstate );

    QString msg;
    switch( newstate )
    {
        case CHECKING:
        {
            msg = tr( "Checking" );
            break;
        }
        case FETCHING:
        {
            msg = tr( "Syncing" );
            break;
        }
        case PARSING:
        {
            msg = tr( "Importing" );
            break;
        }
        case SCANNING:
        {
            msg = tr( "Scanning (%L1 tracks)" ).arg( info );
            break;
        }
        case SYNCED:
        {
            msg = QString();
            break;
        }

        default:
            msg = QString();
    }

    d->state = newstate;
    d->textStatus = msg;
    emit stateChanged();
}


unsigned int
Source::trackCount() const
{
    Q_D( const Source );

    return d->stats.value( "numfiles", 0 ).toUInt();
}

query_ptr
Source::currentTrack() const
{
    Q_D( const Source );

    return d->currentTrack;
}


Tomahawk::playlistinterface_ptr
Source::playlistInterface()
{
    Q_D( Source );

    if ( d->playlistInterface.isNull() )
    {
        Tomahawk::source_ptr source = SourceList::instance()->get( id() );
        d->playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::SourcePlaylistInterface( source.data() ) );
    }

    return d->playlistInterface;
}

QSharedPointer<QMutexLocker>
Source::acquireLock()
{
    Q_D( Source );

    return QSharedPointer<QMutexLocker>( new QMutexLocker( &d->mutex ) );
}


void
Source::onPlaybackStarted( const Tomahawk::track_ptr& track, unsigned int duration )
{
    Q_D( Source );

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << track->toString();

    d->currentTrack = track->toQuery();
    d->currentTrackTimer.start( duration * 1000 + 900000 ); // duration comes in seconds

    if ( d->playlistInterface.isNull() )
        playlistInterface();

    emit playbackStarted( track );
    emit stateChanged();
}


void
Source::onPlaybackFinished( const Tomahawk::track_ptr& track, const Tomahawk::PlaybackLog& log )
{
    Q_D( Source );

    tDebug() << Q_FUNC_INFO << track->toString();
    emit playbackFinished( track, log );

    d->currentTrack.clear();
    emit stateChanged();
}


void
Source::trackTimerFired()
{
    Q_D( Source );

    d->currentTrack.clear();
    emit stateChanged();
}


QString
Source::lastCmdGuid() const
{
    Q_D( const Source );

    QMutexLocker lock( &d->cmdMutex );
    return d->lastCmdGuid;
}


void
Source::setLastCmdGuid( const QString& guid )
{
    Q_D( Source );

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "name is" << friendlyName() << "and guid is" << guid;

    QMutexLocker lock( &d->cmdMutex );
    d->lastCmdGuid = guid;
}


void
Source::addCommand( const dbcmd_ptr& command )
{
    Q_D( Source );

    QMutexLocker lock( &d->cmdMutex );

    d->cmds << command;
    if ( !command->singletonCmd() )
    {
        d->lastCmdGuid = command->guid();
    }

    d->commandCount = d->cmds.count();
}


void
Source::executeCommands()
{
    Q_D( Source );

    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "executeCommands", Qt::QueuedConnection );
        return;
    }

    bool commandsAvail = false;
    {
        QMutexLocker lock( &d->cmdMutex );
        commandsAvail = !d->cmds.isEmpty();
    }

    if ( commandsAvail )
    {
        QMutexLocker lock( &d->cmdMutex );
        QList< Tomahawk::dbcmd_ptr > cmdGroup;
        Tomahawk::dbcmd_ptr cmd = d->cmds.takeFirst();
        while ( cmd->groupable() )
        {
            cmdGroup << cmd;
            if ( !d->cmds.isEmpty() && d->cmds.first()->groupable() && d->cmds.first()->commandname() == cmd->commandname() )
                cmd = d->cmds.takeFirst();
            else
                break;
        }

        // return here when the last command finished
        connect( cmd.data(), SIGNAL( finished() ), SLOT( executeCommands() ) );

        if ( cmdGroup.count() )
        {
            Database::instance()->enqueue( cmdGroup );
        }
        else
        {
            Database::instance()->enqueue( cmd );
        }

        int percentage = ( float( d->commandCount - d->cmds.count() ) / (float)d->commandCount ) * 100.0;
        d->textStatus = tr( "Saving (%1%)" ).arg( percentage );
        emit stateChanged();
    }
    else
    {
        if ( d->updateIndexWhenSynced )
        {
            d->updateIndexWhenSynced = false;
            updateTracks();
        }

        d->textStatus = QString();
        d->state = SYNCED;

        emit commandsFinished();
        emit stateChanged();
        emit synced();
    }
}


void
Source::reportSocialAttributesChanged( DatabaseCommand_SocialAction* action )
{
    Q_ASSERT( action );

    emit socialAttributesChanged( action->action() );

    if ( action->action() == "latchOn" )
    {
        const source_ptr to = SourceList::instance()->get( action->comment() );
        if ( !to.isNull() )
            emit latchedOn( to );
    }
    else if ( action->action() == "latchOff" )
    {
        const source_ptr from = SourceList::instance()->get( action->comment() );
        if ( !from.isNull() )
            emit latchedOff( from );
    }
}


void
Source::updateTracks()
{
    {
        DatabaseCommand* cmd = new DatabaseCommand_UpdateSearchIndex();
        Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
    }

    {
        // Re-calculate local db stats
        DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( SourceList::instance()->get( id() ) );
        connect( cmd, SIGNAL( done( QVariantMap ) ), SLOT( setStats( QVariantMap ) ), Qt::QueuedConnection );
        Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
    }
}


void
Source::updateIndexWhenSynced()
{
    Q_D( Source );

    d->updateIndexWhenSynced = true;
}


QString
Source::textStatus() const
{
    Q_D( const Source );

    if ( !d->textStatus.isEmpty() )
    {
        return d->textStatus;
    }

    if ( !currentTrack().isNull() )
    {
        return currentTrack()->queryTrack()->artist() + " - " + currentTrack()->queryTrack()->track();
    }

    // do not use isOnline() here - it will always return true for the local source
    if ( d->online )
    {
        return tr( "Online" );
    }
    else
    {
        return tr( "Offline" );
    }
}

DBSyncConnectionState
Source::state() const
{
    Q_D( const Source );

    return d->state;
}
