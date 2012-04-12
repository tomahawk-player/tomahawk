/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#include "SpotifyAccount.h"
#include "playlist.h"
#include "playlist/PlaylistUpdaterInterface.h"
#include "sourcelist.h"
#include "SpotifyAccountConfig.h"
#include "SpotifyPlaylistUpdater.h"
#include "resolvers/scriptresolver.h"
#include "utils/tomahawkutils.h"
#include "actioncollection.h"


#include <QPixmap>
#include <QAction>

using namespace Tomahawk;
using namespace Accounts;

static QPixmap* s_icon = 0;

Account*
SpotifyAccountFactory::createAccount( const QString& accountId )
{
    return new SpotifyAccount( accountId );
}


bool
SpotifyAccountFactory::acceptsPath( const QString& path ) const
{
    QFileInfo info( path );
    return info.baseName().startsWith( "spotify_" );
}


Account*
SpotifyAccountFactory::createFromPath( const QString& path )
{
    return new SpotifyAccount( generateId( factoryId() ), path );
}


QPixmap
SpotifyAccountFactory::icon() const
{
    if ( !s_icon )
        s_icon = new QPixmap( RESPATH "images/spotify-logo.png" );

    return *s_icon;
}


SpotifyAccount::SpotifyAccount( const QString& accountId )
    : ResolverAccount( accountId )
{
    init();
}


SpotifyAccount::SpotifyAccount( const QString& accountId, const QString& path )
    : ResolverAccount( accountId, path )
{
    init();
}


SpotifyAccount::~SpotifyAccount()
{
    foreach( QAction* action, m_customActions )
        ActionCollection::instance()->removeAction( action );
}


void
SpotifyAccount::init()
{
    qRegisterMetaType< Tomahawk::Accounts::SpotifyPlaylistInfo* >( "Tomahawk::Accounts::SpotifyPlaylist*" );

    m_spotifyResolver = dynamic_cast< ScriptResolver* >( m_resolver.data() );

    connect( m_spotifyResolver.data(), SIGNAL( customMessage( QString,QVariantMap ) ), this, SLOT( resolverMessage( QString, QVariantMap ) ) );

    const bool hasMigrated = configuration().value( "hasMigrated" ).toBool();
    if ( !hasMigrated )
    {
        qDebug() << "Getting credentials from spotify resolver to migrate to in-app config";
        QVariantMap msg;
        msg[ "_msgtype" ] = "getCredentials";
        m_spotifyResolver.data()->sendMessage( msg );
    }

    QAction* action = new QAction( 0 );
    action->setIcon( QIcon( RESPATH "images/spotify-logo.png" ) );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( syncActionTriggered( bool ) ) );
    ActionCollection::instance()->addAction( ActionCollection::LocalPlaylists, action, this );
    m_customActions.append( action );
}


void
SpotifyAccount::aboutToShow( QAction* action, const playlist_ptr& playlist )
{
    if ( !m_customActions.contains( action ) )
        return;

    // If it's not being synced, allow the option to sync
    SpotifyPlaylistUpdater* updater = qobject_cast< SpotifyPlaylistUpdater* >( playlist->updater() );
    if ( !updater || !updater->sync() )
    {
        action->setText( tr( "Sync with Spotify" ) );
    }
    else
    {
        action->setText( tr( "Stop syncing with Spotify" ) );
    }
}


void
SpotifyAccount::syncActionTriggered( bool checked )
{
    Q_UNUSED( checked );
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( !action || !m_customActions.contains( action ) )
        return;

    const playlist_ptr playlist = action->property( "payload" ).value< playlist_ptr >();
    if ( playlist.isNull() )
    {
        qWarning() << "Got context menu spotify sync action triggered, but invalid playlist payload!";
        Q_ASSERT( false );
        return;
    }

    SpotifyPlaylistUpdater* updater = qobject_cast< SpotifyPlaylistUpdater* >( playlist->updater() );

    if ( !updater )
    {
        QVariantMap msg;
        msg[ "_msgtype" ] = "createPlaylist";
        msg[ "sync" ] = true;
        msg[ "title" ] = playlist->title();

        QList< query_ptr > queries;
        foreach ( const plentry_ptr& ple, playlist->entries() )
            queries << ple->query();
        QVariantList tracks = SpotifyPlaylistUpdater::queriesToVariant( queries );
        msg[ "tracks" ] = tracks;

        const QString qid = sendMessage( msg, this, "playlistCreated" );
        m_waitingForCreateReply[ qid ] = playlist;
    }
    else
    {
        SpotifyPlaylistInfo* info = 0;
        foreach ( SpotifyPlaylistInfo* ifo, m_allSpotifyPlaylists )
        {
            if ( ifo->plid == updater->spotifyId() )
            {
                info = ifo;
                break;
            }
        }

        Q_ASSERT( info );

        if ( !updater->sync() )
        {
            if ( info )
                info->sync = true;
            if ( m_configWidget.data() )
                m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists );

            startPlaylistSync( info );
        }
        else
        {
            stopPlaylistSync( info, true );
        }
    }
}


void
SpotifyAccount::resolverMessage( const QString &msgType, const QVariantMap &msg )
{
    if ( msgType == "credentials" )
    {
        QVariantHash creds = credentials();
        creds[ "username" ] = msg.value( "username" );
        creds[ "password" ] = msg.value( "password" );
        creds[ "highQuality" ] = msg.value( "highQuality" );
        setCredentials( creds );

        qDebug() << "Set creds:" << creds.value( "username" ) << creds.value( "password" ) << msg.value( "username" ) << msg.value( "password" );

        QVariantHash config = configuration();
        config[ "hasMigrated" ] = true;
        setConfiguration( config );
        sync();

        return;
    }


    const QString qid = msg.value( "qid" ).toString();
    if ( m_qidToSlotMap.contains( qid ) )
    {
        QObject* receiver = m_qidToSlotMap[ qid ].first;
        QString slot = m_qidToSlotMap[ qid ].second;
        m_qidToSlotMap.remove( qid );

        QMetaObject::invokeMethod( receiver, slot.toLatin1(), Q_ARG( QString, msgType ), Q_ARG( QVariantMap, msg ) );
    }
    else if ( msgType == "allPlaylists" )
    {
        const QVariantList playlists = msg.value( "playlists" ).toList();
        qDeleteAll( m_allSpotifyPlaylists );
        m_allSpotifyPlaylists.clear();

        foreach ( const QVariant& playlist, playlists )
        {
            const QVariantMap plMap = playlist.toMap();
            const QString name = plMap.value( "name" ).toString();
            const QString plid = plMap.value( "id" ).toString();
            const QString revid = plMap.value( "revid" ).toString();
            const bool sync = plMap.value( "sync" ).toBool();

            if ( name.isNull() || plid.isNull() || revid.isNull() )
            {
                qDebug() << "Did not get name and plid and revid for spotify playlist:" << name << plid << revid << plMap;
                continue;
            }
            m_allSpotifyPlaylists << new SpotifyPlaylistInfo( name, plid, revid, sync );
        }

        if ( !m_configWidget.isNull() )
        {
            m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists );
        }
    }
    else if ( msgType == "tracksAdded" )
    {
        const QString plid = msg.value( "playlistid" ).toString();
        // We should already be syncing this playlist if we get updates for it
//         Q_ASSERT( m_updaters.contains( plid ) );

        if ( !m_updaters.contains( plid ) )
            return;

        SpotifyPlaylistUpdater* updater = m_updaters[ plid ];
        Q_ASSERT( updater->sync() );

        const QString startPos = msg.value( "startPosition" ).toString();
        const QVariantList tracksList = msg.value( "tracks" ).toList();
        const QString newRev = msg.value( "revid" ).toString();
        const QString oldRev = msg.value( "oldRev" ).toString();

        updater->spotifyTracksAdded( tracksList, startPos, newRev, oldRev  );
    }
    else if ( msgType == "tracksRemoved" )
    {
        const QString plid = msg.value( "playlistid" ).toString();
        // We should already be syncing this playlist if we get updates for it
//         Q_ASSERT( m_updaters.contains( plid ) );

        if ( !m_updaters.contains( plid ) )
            return;

        SpotifyPlaylistUpdater* updater = m_updaters[ plid ];

        // If we're not syncing with this, the resolver is quite misinformed.
//         Q_ASSERT( updater && updater->sync() );
        if ( !updater || !updater->sync() )
            return;

        const QVariantList tracksList = msg.value( "trackPositions" ).toList();
        const QString newRev = msg.value( "revid" ).toString();
        const QString oldRev = msg.value( "oldRev" ).toString();


        updater->spotifyTracksRemoved( tracksList, newRev, oldRev );
    }
    else if ( msgType == "tracksMoved" )
    {
        const QString plid = msg.value( "playlistid" ).toString();
        // We should already be syncing this playlist if we get updates for it
        Q_ASSERT( m_updaters.contains( plid ) );

        if ( !m_updaters.contains( plid ) )
            return;

        SpotifyPlaylistUpdater* updater = m_updaters[ plid ];
        Q_ASSERT( updater->sync() );

        const QVariantList tracksList = msg.value( "trackPositions" ).toList();
        const QString newRev = msg.value( "revid" ).toString();
        const QString oldRev = msg.value( "oldRev" ).toString();

        updater->spotifyTracksMoved( tracksList, newRev, oldRev  );
    }
    else if( msgType == "playlistRenamed" )
    {
        const QString plid = msg.value( "id" ).toString();
        // We should already be syncing this playlist if we get updates for it
        //Q_ASSERT( m_updaters.contains( plid ) );

        qDebug() << Q_FUNC_INFO;
        if ( !m_updaters.contains( plid ) )
            return;

        SpotifyPlaylistUpdater* updater = m_updaters[ plid ];
        Q_ASSERT( updater->sync() );

        qDebug() << "Playlist renamed fetched in tomahawk";
        const QString title = msg.value( "name" ).toString();
        const QString newRev = msg.value( "revid" ).toString();
        const QString oldRev = msg.value( "oldRev" ).toString();

        updater->spotifyPlaylistRenamed( title, newRev, oldRev  );
    }
}


QPixmap
SpotifyAccount::icon() const
{
    if ( !s_icon )
        s_icon = new QPixmap( RESPATH "images/spotify-logo.png" );

    return *s_icon;
}


QWidget*
SpotifyAccount::configurationWidget()
{
    if ( m_configWidget.isNull() )
    {
        m_configWidget = QWeakPointer< SpotifyAccountConfig >( new SpotifyAccountConfig( this ) );
        m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists );
    }

    return static_cast< QWidget* >( m_configWidget.data() );
}


void
SpotifyAccount::saveConfig()
{
    Q_ASSERT( !m_configWidget.isNull() );
    if ( m_configWidget.isNull() )
        return;

    QVariantHash creds = credentials();
    if ( creds.value( "username" ).toString() != m_configWidget.data()->username() ||
         creds.value( "password" ).toString() != m_configWidget.data()->password() ||
         creds.value( "highQuality" ).toBool() != m_configWidget.data()->highQuality() )
    {
        creds[ "username" ] = m_configWidget.data()->username();
        creds[ "password" ] = m_configWidget.data()->password();
        creds[ "highQuality" ] = m_configWidget.data()->highQuality();
        setCredentials( creds );

        // Send the result to the resolver
        QVariantMap msg;
        msg[ "_msgtype" ] = "saveSettings";
        msg[ "username" ] = m_configWidget.data()->username();
        msg[ "password" ] = m_configWidget.data()->password();
        msg[ "highQuality" ] = m_configWidget.data()->highQuality();

        m_spotifyResolver.data()->sendMessage( msg );
    }

    QVariantHash config = configuration();
    config[ "deleteOnUnsync" ] = m_configWidget.data()->deleteOnUnsync();
    setConfiguration( config );

    m_configWidget.data()->saveSettings();
    foreach ( SpotifyPlaylistInfo* pl, m_allSpotifyPlaylists )
    {
//        qDebug() << "Checking changed state:" << pl->changed << pl->name << pl->sync;
        if ( pl->changed )
        {
            pl->changed = false;
            if ( pl->sync )
            {
                // Fetch full playlist contents, then begin the sync
                startPlaylistSync( pl );
            }
            else
                stopPlaylistSync( pl );
        }
    }
    sync();
}


void
SpotifyAccount::startPlaylistSync( SpotifyPlaylistInfo* playlist )
{
    if ( !playlist )
        return;

    QVariantMap msg;
    msg[ "_msgtype" ] = "getPlaylist";
    msg[ "playlistid" ] = playlist->plid;
    msg[ "sync" ] = playlist->sync;

    sendMessage( msg, this, "startPlaylistSyncWithPlaylist" );
}


void
SpotifyAccount::startPlaylistSyncWithPlaylist( const QString& msgType, const QVariantMap& msg )
{
    Q_UNUSED( msgType );
    qDebug() << Q_FUNC_INFO <<  "Got full spotify playlist body, creating a tomahawk playlist and enabling sync!!";
    const QString id = msg.value( "id" ).toString();
    const QString name = msg.value( "name" ).toString();
    const QString revid = msg.value( "revid" ).toString();

    qDebug() << "Starting sync with pl:" << id << name;
    QVariantList tracks = msg.value( "tracks" ).toList();

    // create a list of query/plentries directly
    QList< query_ptr > queries = SpotifyPlaylistUpdater::variantToQueries( tracks );

    /**
     * Begin syncing a playlist. Two options:
     * 1) This is a playlist that has never been synced to tomahawk. Create a new one
     *    and attach a new SpotifyPlaylistUpdater to it
     * 2) This was previously synced, and has since been unsynced. THe playlist is still around
     *    with an inactive SpotifyPlaylistUpdater, so just enable it and bring it up to date by merging current with new
     *    TODO: show a warning( "Do you want to overwrite with spotify's version?" )
     */
    if ( m_updaters.contains( id ) )
    {
        Q_ASSERT( m_updaters[ id ]->sync() == false ); /// Should have been unchecked/off before
        m_updaters[ id ]->setSync( true );
//         m_updaters[ id ]->
        // TODO
    }
    else
    {
        playlist_ptr plPtr = Tomahawk::Playlist::create( SourceList::instance()->getLocal(),
                                                        uuid(),
                                                        name,
                                                        QString(),
                                                        QString(),
                                                        false,
                                                        queries );

        SpotifyPlaylistUpdater* updater = new SpotifyPlaylistUpdater( this, revid, id, plPtr );
        updater->setSync( true );
        m_updaters[ id ] = updater;
    }
}


void
SpotifyAccount::playlistCreated( const QString& msgType, const QVariantMap& msg )
{
    Q_UNUSED( msgType );

    qDebug() << Q_FUNC_INFO << "Got response from our createPlaylist command, now creating updater and attaching";
    const bool success = msg.value( "success" ).toBool();

    if ( !success )
    {
        qWarning() << "Got FAILED return code from spotify resolver createPlaylist command, aborting sync";
        return;
    }

    const QString id = msg.value( "playlistid" ).toString();
    const QString revid = msg.value( "playlistid" ).toString();
    const QString qid = msg.value( "qid" ).toString();

    if ( !m_waitingForCreateReply.contains( qid ) )
    {
        qWarning() << "Got a createPlaylist reply for a playlist/qid we were not waiting for :-/ " << qid << m_waitingForCreateReply;
        return;
    }

    playlist_ptr playlist = m_waitingForCreateReply.take( qid );
    SpotifyPlaylistUpdater* updater = new SpotifyPlaylistUpdater( this, revid, id, playlist );
    updater->setSync( true );
    m_updaters[ id ] = updater;
}


QString
SpotifyAccount::sendMessage( const QVariantMap &m, QObject* obj, const QString& slot )
{
    QVariantMap msg = m;
    QString qid;

    if ( obj )
    {
        qid = QUuid::createUuid().toString().replace( "{", "" ).replace( "}", "" );

        m_qidToSlotMap[ qid ] = qMakePair( obj, slot );
        msg[ "qid" ] = qid;

    }

    m_spotifyResolver.data()->sendMessage( msg );

    return qid;
}


void
SpotifyAccount::registerUpdaterForPlaylist( const QString& plId, SpotifyPlaylistUpdater* updater )
{
    m_updaters[ plId ] = updater;
}


void
SpotifyAccount::unregisterUpdater( const QString& plid )
{
    m_updaters.remove( plid );
}


void
SpotifyAccount::fetchFullPlaylist( SpotifyPlaylistInfo* playlist )
{

}


bool
SpotifyAccount::deleteOnUnsync() const
{
    return configuration().value( "deleteOnUnsync", false ).toBool();
}

void
SpotifyAccount::stopPlaylistSync( SpotifyPlaylistInfo* playlist, bool forceDontDelete )
{
    if ( !playlist )
        return;

    QVariantMap msg;
    msg[ "_msgtype" ] = "removeFromSyncList";
    msg[ "playlistid" ] = playlist->plid;

    m_spotifyResolver.data()->sendMessage( msg );

    if ( m_updaters.contains( playlist->plid ) )
    {
        SpotifyPlaylistUpdater* updater = m_updaters[ playlist->plid ];
        updater->setSync( false );

        if ( deleteOnUnsync() && !forceDontDelete )
        {
            playlist_ptr tomahawkPl = updater->playlist();

            if ( !tomahawkPl.isNull() )
                Playlist::remove( tomahawkPl );

            updater->deleteLater();

        }

        updater->save();
    }
}



void
SpotifyAccount::loadPlaylists()
{
    // TODO cache this and only get changed?
    QVariantMap msg;
    msg[ "_msgtype" ] = "getAllPlaylists";
    sendMessage( msg, this, "allPlaylistsLoaded" );
}


void
SpotifyAccount::setSyncForPlaylist( const QString& spotifyPlaylistId, bool sync )
{
    foreach ( SpotifyPlaylistInfo* info, m_allSpotifyPlaylists )
    {
        if( info->plid == spotifyPlaylistId )
            info->sync = sync;
    }

    if ( !m_configWidget.isNull() )
        m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists );
}

