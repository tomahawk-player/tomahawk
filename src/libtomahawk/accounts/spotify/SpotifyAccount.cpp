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
#include "Playlist.h"
#include "playlist/PlaylistUpdaterInterface.h"
#include "SourceList.h"
#include "SpotifyAccountConfig.h"
#include "SpotifyPlaylistUpdater.h"
#include "resolvers/ScriptResolver.h"
#include "utils/TomahawkUtilsGui.h"
#include "ActionCollection.h"
#include "Pipeline.h"
#include "accounts/AccountManager.h"
#include "utils/Closure.h"
#include "SpotifyInfoPlugin.h"
#include "infosystem/InfoSystem.h"

#ifndef ENABLE_HEADLESS
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#endif

#include <QPixmap>
#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QCoreApplication>

using namespace Tomahawk;
using namespace Accounts;


static QPixmap* s_icon = 0;

#ifdef Q_OS_MAC
static QString s_resolverId = "spotify-osx";
#elif defined(Q_OS_WIN)
static QString s_resolverId = "spotify-win";
#elif defined(Q_OS_LINUX) && defined(__GNUC__) && defined(__x86_64__)
static QString s_resolverId = "spotify-linux-x64";
#elif defined(Q_OS_LINUX)
static QString s_resolverId = "spotify-linux-x86";
#else
static QString s_resolverId = "spotify-unknown";
#endif


namespace {
enum ActionType {
    Sync = 0,
    Subscribe = 1,
    Collaborate
};
}

Account*
SpotifyAccountFactory::createAccount( const QString& accountId )
{
    return new SpotifyAccount( accountId );
}


QPixmap
SpotifyAccountFactory::icon() const
{
    if ( !s_icon )
        s_icon = new QPixmap( RESPATH "images/spotify-logo.png" );

    return *s_icon;
}


SpotifyAccount* SpotifyAccount::s_instance = 0;

SpotifyAccount*
SpotifyAccount::instance()
{
    return s_instance;
}

SpotifyAccount::SpotifyAccount( const QString& accountId )
    : CustomAtticaAccount( accountId )
    , m_preventEnabling( false )
    , m_loggedIn( false )
{
    init();
}


SpotifyAccount::~SpotifyAccount()
{
    clearUser();

    if ( m_spotifyResolver.isNull() )
        return;

    Pipeline::instance()->removeScriptResolver( m_spotifyResolver.data()->filePath() );
    delete m_spotifyResolver.data();
}


void
SpotifyAccount::init()
{
    setAccountFriendlyName( "Spotify" );
    setAccountServiceName( "spotify" );

    AtticaManager::instance()->registerCustomAccount( s_resolverId, this );
    qRegisterMetaType< Tomahawk::Accounts::SpotifyPlaylistInfo* >( "Tomahawk::Accounts::SpotifyPlaylist*" );

    if ( infoPlugin() && Tomahawk::InfoSystem::InfoSystem::instance()->workerThread() )
    {
        infoPlugin().data()->moveToThread( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() );
        Tomahawk::InfoSystem::InfoSystem::instance()->addInfoPlugin( infoPlugin() );
    }

    if ( !AtticaManager::instance()->resolversLoaded() )
    {
        // If we're still waiting to load, wait for the attica resolvers to come down the pipe
        connect( AtticaManager::instance(), SIGNAL( resolversLoaded( Attica::Content::List ) ), this, SLOT( delayedInit() ), Qt::UniqueConnection );
    }
    else
    {
        delayedInit();
    }
}


void
SpotifyAccount::delayedInit()
{
    connect( AtticaManager::instance(), SIGNAL( resolverInstalled( QString ) ), this, SLOT( resolverInstalled( QString ) ) );

    const Attica::Content res = AtticaManager::instance()->resolverForId( s_resolverId );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );

    const QString path = configuration().value( "path" ).toString(); // Manual path override
    if ( !checkForResolver() && state != AtticaManager::Uninstalled )
    {
        // If the user manually deleted the resolver, mark it as uninstalled, so we re-fetch for the user
        QVariantHash conf = configuration();
        conf.remove( "path" );
        setConfiguration( conf );
        sync();

        AtticaManager::instance()->uninstallResolver( res );
    }
    else if ( state == AtticaManager::Installed || !path.isEmpty() )
    {
        if ( !path.isEmpty() )
        {
            QFileInfo info( path );
            // Resolver was deleted, so abort and remove our manual override, as it's no longer valid
            if ( !info.exists() )
            {
                QVariantHash conf = configuration();
                conf.remove( "path" );
                setConfiguration( conf );
                sync();
                return;
            }
        }
        hookupResolver();
    }
}

void
SpotifyAccount::hookupResolver()
{
    // initialize the resolver itself. this is called if the account actually has an installed spotify resolver,
    // as it might not.
    // If there is a spotify resolver from attica installed, create the corresponding ExternalResolver* and hook up to it
    QString path = configuration().value( "path" ).toString();
    if ( path.isEmpty() )
    {
        const Attica::Content res = AtticaManager::instance()->resolverForId( s_resolverId );
        const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );
        Q_ASSERT( state == AtticaManager::Installed );
        Q_UNUSED( state );

        const AtticaManager::Resolver data = AtticaManager::instance()->resolverData( res.id() );
        path = data.scriptPath;
    }

    qDebug() << "Starting spotify resolver with path:" << path;
    if ( !m_spotifyResolver.isNull() )
    {
        delete m_spotifyResolver.data();
    }

    if ( !QFile::exists( path ) )
    {
        qWarning() << "Asked to hook up spotify resolver but it doesn't exist, ignoring";
        return;
    }

    // HACK
    // Since the resolver in 0.4.x used an incompatible version of kdsingleappguard, we can't auto-kill old resolvers on the
    // 0.4.x->0.5.x upgrade. So we do it manually for a while
    killExistingResolvers();
    m_spotifyResolver = QWeakPointer< ScriptResolver >( qobject_cast< ScriptResolver* >( Pipeline::instance()->addScriptResolver( path ) ) );
    m_spotifyResolver.data()->setIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::SpotifyIcon ) );

    connect( m_spotifyResolver.data(), SIGNAL( changed() ), this, SLOT( resolverChanged() ) );
    connect( m_spotifyResolver.data(), SIGNAL( customMessage( QString,QVariantMap ) ), this, SLOT( resolverMessage( QString, QVariantMap ) ) );

    // Always get logged in status
    QVariantMap msg;
    msg[ "_msgtype" ] = "getCredentials";
    m_spotifyResolver.data()->sendMessage( msg );
}


void
SpotifyAccount::killExistingResolvers()
{
    QProcess p;
#if defined(Q_OS_UNIX)
    const int ret = p.execute( "killall -9 spotify_tomahawkresolver" );
    qDebug() << "Tried to killall -9 spotify_tomahawkresolver with return code:" << ret;
#elif defined(Q_OS_WIN)
    const int ret = p.execute( "taskkill.exe /F /im spotify_tomahawkresolver.exe" );
    qDebug() << "Tried to taskkill.exe /F /im spotify_tomahawkresolver.exe with return code:" << ret;
#endif
}


bool
SpotifyAccount::checkForResolver()
{
#if defined(Q_OS_WIN)
    QDir appDataDir = TomahawkUtils::appDataDir();
    return appDataDir.exists( QString( "atticaresolvers/%1/spotify_tomahawkresolver.exe" ).arg( s_resolverId ) );
#elif defined(Q_OS_LINUX)  || defined(Q_OS_MAC)
    QDir appDataDir = TomahawkUtils::appDataDir();
    return appDataDir.exists( QString( "atticaresolvers/%1/spotify_tomahawkresolver" ).arg( s_resolverId ) );
#endif

    return false;
}

void
SpotifyAccount::resolverChanged()
{
    emit connectionStateChanged( connectionState() );
}


Attica::Content
SpotifyAccount::atticaContent() const
{
    return AtticaManager::instance()->resolverForId( s_resolverId );
}


void
SpotifyAccount::authenticate()
{
    if ( !AtticaManager::instance()->resolversLoaded() )
    {
        // If we're still waiting to load, wait for the attica resolvers to come down the pipe
        connect( AtticaManager::instance(), SIGNAL( resolversLoaded( Attica::Content::List ) ), this, SLOT( atticaLoaded( Attica::Content::List ) ), Qt::UniqueConnection );
        return;
    }

    const Attica::Content res = AtticaManager::instance()->resolverForId( s_resolverId );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );

    qDebug() << "Spotify account authenticating...";

    const QString path = configuration().value( "path" ).toString();
    const QFileInfo info( path );
    const bool manualResolverRemoved = !path.isEmpty() && !info.exists();

    if ( m_spotifyResolver.isNull() && state == AtticaManager::Installed )
    {
        // We don;t have the resolver but it has been installed via attica already, so lets just turn it on
        qDebug() << "No valid spotify resolver running, but attica reports it is installed, so start it up";
        hookupResolver();
    }
    else if ( m_spotifyResolver.isNull() || manualResolverRemoved )
    {
        qDebug() << "Got null resolver but asked to authenticate, so installing if we have one from attica:" << res.isValid() << res.id();
        if ( res.isValid() && !res.id().isEmpty() )
            AtticaManager::instance()->installResolver( res, false );
        else
        {
#ifdef Q_OS_LINUX
            m_preventEnabling = true;
#endif
        }
    }
    else if ( !m_spotifyResolver.data()->running() )
    {
        qDebug() << "Spotify resolver exists but stopped, starting";
        m_spotifyResolver.data()->start();
    }
    else
    {
        qDebug() << "Spotify resolver exists and is running, ignore authentication attempt";
    }

    emit connectionStateChanged( connectionState() );
}


void
SpotifyAccount::deauthenticate()
{
    if ( !m_spotifyResolver.isNull() && m_spotifyResolver.data()->running() )
        m_spotifyResolver.data()->stop();

    emit connectionStateChanged( connectionState() );
}


bool
SpotifyAccount::isAuthenticated() const
{
    return !m_spotifyResolver.isNull() && m_spotifyResolver.data()->running();
}


Account::ConnectionState
SpotifyAccount::connectionState() const
{
    return (!m_spotifyResolver.isNull() && m_spotifyResolver.data()->running()) ? Account::Connected : Account::Disconnected;
}


InfoSystem::InfoPluginPtr
SpotifyAccount::infoPlugin()
{
    if ( m_infoPlugin.isNull() )
    {
        m_infoPlugin = QWeakPointer< InfoSystem::SpotifyInfoPlugin >( new InfoSystem::SpotifyInfoPlugin( this ) );
    }

    return InfoSystem::InfoPluginPtr( m_infoPlugin.data() );
}


void
SpotifyAccount::resolverInstalled(const QString& resolverId)
{
    if ( resolverId == s_resolverId )
    {
        // We requested this install, so we want to launch it
        hookupResolver();

        if ( enabled() )
            authenticate();
        else
            AccountManager::instance()->enableAccount( this );
    }
}


void
SpotifyAccount::atticaLoaded( Attica::Content::List )
{
    disconnect( AtticaManager::instance(), SIGNAL( resolversLoaded( Attica::Content::List ) ), this, SLOT( atticaLoaded( Attica::Content::List ) ) );
    authenticate();
}


void
SpotifyAccount::setManualResolverPath( const QString &resolverPath )
{
    Q_ASSERT( !resolverPath.isEmpty() );

    QVariantHash conf = configuration();
    conf[ "path" ] = resolverPath;
    setConfiguration( conf );
    sync();

    // uninstall
    const Attica::Content res = AtticaManager::instance()->resolverForId( s_resolverId );
    if ( AtticaManager::instance()->resolverState( res ) != AtticaManager::Uninstalled )
        AtticaManager::instance()->uninstallResolver( res );

    m_preventEnabling = false;

    if ( !m_spotifyResolver.isNull() )
    {
        // replace
        AccountManager::instance()->disableAccount( this );
        NewClosure( m_spotifyResolver.data(), SIGNAL( destroyed() ), this, SLOT( hookupAfterDeletion( bool ) ), true );
        m_spotifyResolver.data()->deleteLater();
    }
    else
    {
        hookupResolver();
        AccountManager::instance()->enableAccount( this );
    }
}

void
SpotifyAccount::starTrack(const QString &artist, const QString &title, const bool starred)
{
    qDebug() << Q_FUNC_INFO << artist << title << starred;
    QVariantMap msg;
    msg[ "_msgtype" ] = "setStarred";
    msg[ "starred" ] = starred;
    msg[ "artist" ] = artist;
    msg[ "title" ] = title;
    sendMessage( msg );
}


bool
SpotifyAccount::loggedIn() const
{
    return m_loggedIn;
}


void
SpotifyAccount::hookupAfterDeletion( bool autoEnable )
{
    hookupResolver();
    if ( autoEnable )
        AccountManager::instance()->enableAccount( this );
}


void
SpotifyAccount::aboutToShow( QAction* action, const playlist_ptr& playlist )
{
    if ( !m_customActions.contains( action ) )
        return;

    // If it's not being synced, allow the option to sync
    bool found = false;
    bool canSubscribe = false;
    bool isSubscribed = false;
    bool manuallyDisabled = false;
    bool sync = false;
    bool owner = false;
    bool collaborative = false;

    action->setVisible( true );

    QList<PlaylistUpdaterInterface*> updaters = playlist->updaters();
    foreach ( PlaylistUpdaterInterface* updater, updaters )
    {
        if ( SpotifyPlaylistUpdater* spotifyUpdater = qobject_cast< SpotifyPlaylistUpdater* >( updater ) )
        {
            found = true;

            canSubscribe = spotifyUpdater->canSubscribe();
            isSubscribed = spotifyUpdater->subscribed();
            owner = spotifyUpdater->owner();
            collaborative = spotifyUpdater->collaborative();

            if ( !canSubscribe && !spotifyUpdater->sync() )
                manuallyDisabled = true;

            if ( spotifyUpdater->sync() )
                sync = true;

        }
    }

    const ActionType actionType = static_cast< ActionType >( action->data().toInt() );

    if ( actionType == Sync )
    {
        if ( !found )
        {
            action->setText( tr( "Sync with Spotify" ) );
        }
        else if ( manuallyDisabled )
        {
            action->setText( tr( "Re-enable syncing with Spotify" ) );
        }
        else
        {
            // We dont want to sync a subscribeable playlist but if a playlist isnt
            // collaborative, he will loose his changes on next update, thus,
            // we create a new copy of it
            if ( canSubscribe )
                action->setText( tr( "Create local copy") );
            else if ( sync )
                action->setText( tr( "Stop syncing with Spotify" ) );
            else
                action->setVisible( false );
        }
    }

    // User can sync or subscribe on playlist.
    // Sync means creating a new copy of it, subscribe is listening on changes from owner
    if ( actionType == Subscribe )
    {
        if ( found && canSubscribe )
        {
            if ( !isSubscribed )
            {
                action->setText( tr( "Subscribe to playlist changes" ) );
            }
            else if ( manuallyDisabled )
            {
                action->setText( tr( "Re-enable playlist subscription" ) );
            }
            else if ( isSubscribed )
            {
                action->setText( tr( "Stop subscribing to changes" ) );
            }
            else
            {
                // Hide the action, we dont have this option on the playlist
                action->setVisible( false );
            }
        }
        else
        {
            action->setVisible( false );
        }
    }

    // If the user is owner of current playlist, enable collaboration options
    if ( actionType == Collaborate )
    {
        if ( found && owner && !manuallyDisabled )
        {
            if ( !collaborative )
                action->setText( tr( "Enable Spotify collaborations" ) );
            else
                action->setText( tr( "Disable Spotify collaborations" ) );
        }
        else
            action->setVisible( false );
    }
}


SpotifyPlaylistUpdater*
SpotifyAccount::getPlaylistUpdater( const playlist_ptr plptr )
{

    SpotifyPlaylistUpdater* updater = 0;
    QList<PlaylistUpdaterInterface*> updaters = plptr->updaters();
    foreach ( PlaylistUpdaterInterface* u, updaters )
    {
        if ( SpotifyPlaylistUpdater* spotifyUpdater = qobject_cast< SpotifyPlaylistUpdater* >( u ) )
        {
            updater = spotifyUpdater;
        }
    }
    return updater;
}

SpotifyPlaylistUpdater*
SpotifyAccount::getPlaylistUpdater( QObject *sender )
{

    if ( !sender )
    {
        tLog() << "uuh noo, null sender!";
        return 0;
    }

    QAction* senderAction = qobject_cast< QAction* >( sender );

    if ( !senderAction )
    {
        tLog() << "uuh noo, null action!";
        return 0;
    }

    const playlist_ptr playlist = playlistFromAction( senderAction );

    if ( playlist.isNull() )
    {
        qWarning() << "Got context menu spotify action " << senderAction->text() << "triggered, but invalid playlist payload!";
        Q_ASSERT( false );
        return 0;
    }

    SpotifyPlaylistUpdater* updater = 0;
    QList<PlaylistUpdaterInterface*> updaters = playlist->updaters();
    foreach ( PlaylistUpdaterInterface* u, updaters )
    {
        if ( SpotifyPlaylistUpdater* spotifyUpdater = qobject_cast< SpotifyPlaylistUpdater* >( u ) )
        {
            updater = spotifyUpdater;
        }
    }
    return updater;
}

void
SpotifyAccount::subscribeActionTriggered( QAction* action )
{
    SpotifyPlaylistUpdater* updater = getPlaylistUpdater( action );

    Q_ASSERT( updater );
    if ( !updater )
        return;

    Q_ASSERT( updater->playlist() );
    if ( !updater->playlist() )
        return;

    // Toggle subscription status
    setSubscribedForPlaylist( updater->playlist(), !updater->subscribed() );
}


void
SpotifyAccount::collaborateActionTriggered( QAction* action )
{

    SpotifyPlaylistUpdater* updater = getPlaylistUpdater( action );

    if ( !updater )
    {
        tLog() << "No SpotifyPlaylistUpdater in payload slot of triggered action! Uh oh!!";
        return;
    }
    else
    {
        SpotifyPlaylistInfo* info = m_allSpotifyPlaylists.value( updater->spotifyId(), 0 );
        Q_ASSERT( info );

        if ( info->isOwner )
        {
            tLog() << info->name << info->isOwner << info->plid << updater->owner() << updater->collaborative();
            QVariantMap msg;
            msg[ "_msgtype" ] = "setCollaborative";
            msg[ "collaborative" ] = !updater->collaborative();
            msg[ "playlistid" ] = info->plid;
            sendMessage( msg, this );
        }
        else
            tLog() << "cant set collab for this pl, not owner!?" << info->name << info->plid;

    }
}


void
SpotifyAccount::syncActionTriggered( QAction* action )
{
    const playlist_ptr playlist = playlistFromAction( action );

    if ( playlist.isNull() )
    {
        qWarning() << "Got context menu spotify sync action triggered, but invalid playlist payload!";
        Q_ASSERT( false );
        return;
    }

    SpotifyPlaylistUpdater* updater = getPlaylistUpdater( playlist );

    if ( !updater || updater->canSubscribe() )
    {
        QVariantMap msg;
        msg[ "_msgtype" ] = "createPlaylist";
        msg[ "sync" ] = true;

        if ( !updater )
            msg[ "title" ] = playlist->title();
        else
            msg[ "title" ] = "Copy of " + playlist->title();

        QList< query_ptr > queries;
        foreach ( const plentry_ptr& ple, playlist->entries() )
            queries << ple->query();
        QVariantList tracks = SpotifyPlaylistUpdater::queriesToVariant( queries );
        msg[ "tracks" ] = tracks;

        QString qid;
        if ( !updater )
            qid = sendMessage( msg, this, "playlistCreated" );
        else
            qid = sendMessage( msg, this, "playlistCopyCreated" );

        m_waitingForCreateReply[ qid ] = playlist;
    }
    else
    {
        SpotifyPlaylistInfo* info = m_allSpotifyPlaylists.value( updater->spotifyId(), 0 );

        Q_ASSERT( info );
        if ( info )
            info->sync = !updater->sync();

        if ( m_configWidget.data() )
            m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists.values() );

        if ( !updater->sync() )
        {
            startPlaylistSync( info );
        }
        else
        {
            stopPlaylistSync( info, true );
        }
    }
}


void
SpotifyAccount::setSubscribedForPlaylist( const playlist_ptr& playlist, bool subscribed )
{
    SpotifyPlaylistUpdater* updater = getPlaylistUpdater( playlist );

    if ( !updater )
    {
        tLog() << "No SpotifyPlaylistUpdater in payload slot of triggered action! Uh oh!!";
        return;
    }

    SpotifyPlaylistInfo* info = m_allSpotifyPlaylists.value( updater->spotifyId(), 0 );

    // When we unsubscribe, all playlists is resent
    // and we will could loose the SpotifyPlaylistInfo, but all we really need is the id
    if ( updater->spotifyId().isEmpty() )
    {
        tLog() << "No spotify id in updater, WTF?";
        return;
    }

    if ( !info )
    {
        info = new SpotifyPlaylistInfo( playlist->title(),
                                        updater->spotifyId(),
                                        updater->spotifyId(),
                                        false,
                                        false
                                        );

        registerPlaylistInfo( info );
    }

    info->subscribed = subscribed;
    info->sync = subscribed;

    QVariantMap msg;
    msg[ "_msgtype" ] = "setSubscription";
    msg[ "subscribe" ] = info->subscribed;
    msg[ "playlistid" ] = info->plid;

    sendMessage( msg, this );

    updater->setSync( subscribed );
    updater->setSubscribedStatus( subscribed );
}


playlist_ptr
SpotifyAccount::playlistFromAction( QAction* action ) const
{
    if ( !action || !m_customActions.contains( action ) )
        return playlist_ptr();

    return action->property( "payload" ).value< playlist_ptr >();
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

        m_loggedIn = msg.value( "loggedIn", false ).toBool();
        if ( m_loggedIn )
        {
            configurationWidget();

            if ( !m_configWidget.isNull() )
                m_configWidget.data()->loginResponse( true, QString(), creds[ "username" ].toString() );
        }

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

        QVariant extraData;
        if ( m_qidToExtraData.contains( qid ) )
            extraData = m_qidToExtraData.take( qid );

        QMetaObject::invokeMethod( receiver, slot.toLatin1(), Q_ARG( QString, msgType ), Q_ARG( QVariantMap, msg ), Q_ARG( QVariant, extraData ) );
    }
    else if ( msgType == "allPlaylists" )
    {
        const QVariantList playlists = msg.value( "playlists" ).toList();
        qDeleteAll( m_allSpotifyPlaylists.values() );
        m_allSpotifyPlaylists.clear();

        foreach ( const QVariant& playlist, playlists )
        {
            const QVariantMap plMap = playlist.toMap();
            const QString name = plMap.value( "name" ).toString();
            const QString plid = plMap.value( "id" ).toString();
            const QString revid = plMap.value( "revid" ).toString();
            const bool isOwner = plMap.value( "owner" ).toBool();
            const bool sync = plMap.value( "sync" ).toBool();
            const bool subscribed = plMap.value( "subscribed" ).toBool();
            const bool starContainer = ( plMap.value( "starContainer" ).toBool() || name == "Starred Tracks");

            if ( name.isNull() || plid.isNull() || revid.isNull() )
            {
                qDebug() << "Did not get name and plid and revid for spotify playlist:" << name << plid << revid << plMap;
                continue;
            }

            registerPlaylistInfo( new SpotifyPlaylistInfo( name, plid, revid, sync, subscribed, isOwner, starContainer ) );
        }

        if ( !m_configWidget.isNull() )
        {
            m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists.values() );
        }
    }
    else if ( msgType == "tracksAdded" )
    {
        const QString plid = msg.value( "playlistid" ).toString();
        // We should already be syncing this playlist if we get updates for it
//         Q_ASSERT( m_updaters.contains( plid ) );

        if ( !m_updaters.contains( plid ) )
            return;

        SpotifyPlaylistInfo* info = m_allSpotifyPlaylists[ plid ];
        if( (info && info->starContainer ) && loveSync() )
        {
            qDebug() << Q_FUNC_INFO << "SKIPPING" << msgType;
            return;
        }

        SpotifyPlaylistUpdater* updater = m_updaters[ plid ];
        // We have previously sycned starred container, but not anymore.
        // If we added loveSync, its synced in the background
        if( !updater->sync() && m_configWidget.data()->loveSync() )
            return;

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

        SpotifyPlaylistInfo* info = m_allSpotifyPlaylists[ plid ];
        if( (info && info->starContainer ) && loveSync() )
        {
            qDebug() << Q_FUNC_INFO << "SKIPPING" << msgType;
            return;
        }
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

        SpotifyPlaylistInfo* info = m_allSpotifyPlaylists[ plid ];
        if( (info && info->starContainer ) && loveSync() )
        {
            qDebug() << Q_FUNC_INFO << "SKIPPING" << msgType;
            return;
        }
        SpotifyPlaylistUpdater* updater = m_updaters[ plid ];
        Q_ASSERT( updater->sync() );

        const QString newStartPos = msg.value( "newStartPosition" ).toString();
        const QVariantList tracksList = msg.value( "tracks" ).toList();
        const QString newRev = msg.value( "revid" ).toString();
        const QString oldRev = msg.value( "oldRev" ).toString();

        updater->spotifyTracksMoved( tracksList, newStartPos, newRev, oldRev  );
    }
    else if ( msgType == "starredChanged" )
    {
        if ( loveSync() )
        {
            const QVariantList tracksList = msg.value( "tracks" ).toList();
            const bool love = msg.value( "starred" ).toBool();

            QList<query_ptr> qs = SpotifyPlaylistUpdater::variantToQueries( tracksList );
            foreach ( const query_ptr& query, qs )
            {
                query->setLoved( love );
            }
        }
    }
    else if ( msgType == "playlistMetadataChanged" )
    {
        const QString plid = msg.value( "id" ).toString();
        // We should already be syncing this playlist if we get updates for it
        //Q_ASSERT( m_updaters.contains( plid ) );

        qDebug() << Q_FUNC_INFO;
        if ( !m_updaters.contains( plid ) )
            return;

        SpotifyPlaylistInfo* info = m_allSpotifyPlaylists[ plid ];
        if( (info && info->starContainer ) && loveSync() )
        {
            qDebug() << Q_FUNC_INFO << "SKIPPING" << msgType;
            return;
        }
        SpotifyPlaylistUpdater* updater = m_updaters[ plid ];
        Q_ASSERT( updater->sync() );

        const QString title = msg.value( "name" ).toString();
        const QString newRev = msg.value( "revid" ).toString();
        const QString oldRev = msg.value( "oldRev" ).toString();
        const bool collaborative = msg.value( "collaborative" ).toBool();
        const int subscribers = msg.value( "subscribers" ).toInt();

        if( info && info->name != title )
        {
            qDebug() << "Playlist renamed fetched in tomahawk";
            updater->spotifyPlaylistRenamed( title, newRev, oldRev  );
        }

        if( updater->collaborative() != collaborative )
        {
            tLog() << "Setting collaborative!" << collaborative;
            updater->setCollaborative( collaborative );
        }

        if( updater->subscribers() != subscribers )
        {
            tLog() << "Updateing number of subscribers" << subscribers;
            updater->setSubscribers( subscribers );
        }
    }
    else if ( msgType == "spotifyError" )
    {
        const QString error = msg.value( "msg" ).toString();
        if ( error.isEmpty() )
            return;

        if ( msg.value( "isDebugMsg" ).toBool() )
            tDebug( LOGVERBOSE ) << "SpotifyResolverError: " << error;
        else
            JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( QString( "Spotify: %1" ).arg( error ) ) );
    }
    else if ( msgType == "userChanged" )
    {
        const QString rmsg = msg.value( "msg" ).toString();
        clearUser( true );

        if ( m_configWidget.data() )
            m_configWidget.data()->setPlaylists( QList< SpotifyPlaylistInfo* >() );

        qDebug() << "User changed message from spotify:" << rmsg;
    }
    else if ( msgType == "loginResponse" )
    {
        QVariantHash creds = credentials();
        creds[ "username" ] = msg.value( "username" ).toString();
        creds[ "password" ] = msg.value( "password" ).toString();
        creds[ "highQuality" ] = msg.value( "highQuality" ).toString();
        setCredentials( creds );
        sync();

        const bool success = msg.value( "success" ).toBool();

        m_loggedIn = success;

        if ( success )
        {
            createActions();
            s_instance = this;
        }
        configurationWidget(); // ensure it's created so we can set the login button
        if ( m_configWidget.data() )
        {
            const QString message = msg.value( "message" ).toString();
            m_configWidget.data()->loginResponse( success, message, creds[ "username" ].toString() );
        }
    }
    else if ( msgType == "playlistDeleted" )
    {
        const QString plid = msg.value( "playlistid" ).toString();

        if ( !m_updaters.contains( plid ) )
            return;

        SpotifyPlaylistUpdater* updater = m_updaters.take( plid );
        updater->remove( false );
    }
    else if ( msgType == "status" )
    {
        const bool loggedIn = msg.value( "loggedIn" ).toBool();
        const QString username = msg.value( "username" ).toString();

        qDebug() << "Got status message with login info:" << loggedIn << username;

        if ( !loggedIn || username.isEmpty() || credentials().value( "username").toString() != username )
        {
            m_loggedIn = false;
            s_instance = 0;
        }

        QVariantMap msg;
        msg[ "_msgtype" ] = "status";
        msg[ "_status" ] = 1;
        sendMessage( msg );

        return;
    }
}


void
SpotifyAccount::clearUser( bool permanentlyDelete )
{
    foreach( SpotifyPlaylistUpdater* updater, m_updaters.values() )
    {
        if ( permanentlyDelete )
            updater->remove( false );
        else
            updater->deleteLater();
    }

    m_updaters.clear();

    qDeleteAll( m_allSpotifyPlaylists.values() );
    m_allSpotifyPlaylists.clear();

    m_qidToSlotMap.clear();
    m_waitingForCreateReply.clear();

    removeActions();
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
        connect( m_configWidget.data(), SIGNAL( login( QString,QString ) ), this, SLOT( login( QString,QString ) ) );
        connect( m_configWidget.data(), SIGNAL( logout() ), this, SLOT( logout() ) );
        m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists.values() );
    }

    if ( m_spotifyResolver.isNull() || !m_spotifyResolver.data()->running() )
        return 0;

    return static_cast< QWidget* >( m_configWidget.data() );
}


QWidget*
SpotifyAccount::aboutWidget()
{
    if ( m_aboutWidget.isNull() )
    {
        QWidget* w = new QWidget();
        w->hide();

        QHBoxLayout* l = new QHBoxLayout( w );
        QLabel* pm = new QLabel( w );
        pm->setPixmap( QPixmap( RESPATH "images/spotifycore-logo" ) );
        QLabel* text = new QLabel( "This product uses SPOTIFY(R) CORE but is not endorsed, certified or otherwise approved in any way by Spotify. Spotify is the registered trade mark of the Spotify Group.", w );
        text->setWordWrap( true );
        l->addWidget( pm );
        l->addWidget( text );
        w->setLayout( l );
        m_aboutWidget = QWeakPointer< QWidget >( w );
    }

    return m_aboutWidget.data();
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

    }

    QVariantHash config = configuration();
    config[ "deleteOnUnsync" ] = m_configWidget.data()->deleteOnUnsync();
    config[ "loveSync" ] = m_configWidget.data()->loveSync();
    setConfiguration( config );

    m_configWidget.data()->saveSettings();
    foreach ( SpotifyPlaylistInfo* pl, m_allSpotifyPlaylists.values() )
    {
//        qDebug() << "Checking changed state:" << pl->changed << "name:" << pl->name << "sync" << pl->sync << "starred:" << pl->starContainer;
        if ( pl->changed )
        {
            pl->changed = false;

            if ( pl->sync || ( pl->starContainer && loveSync() ) )
            {
                // Fetch full playlist contents, then begin the sync
                startPlaylistSync( pl );
            }
            else
                stopPlaylistSync( pl );
        }
    }
    sync();

    if ( !m_configWidget.data()->loggedInManually() && !m_configWidget.data()->username().isEmpty() && !m_configWidget.data()->password().isEmpty() )
    {
        // If the user never pressed log in, he might have just pressed ok or hit enter. So log in anyway
        login( m_configWidget.data()->username(), m_configWidget.data()->password() );
    }
}


void
SpotifyAccount::login( const QString& username, const QString& password )
{
    QVariantMap msg;
    msg[ "_msgtype" ] = "login";
    msg[ "username" ] = username;
    msg[ "password" ] = password;

    msg[ "highQuality" ] = m_configWidget.data()->highQuality();

    m_spotifyResolver.data()->sendMessage( msg );
}


void
SpotifyAccount::logout()
{
    QVariantMap msg;
    msg[ "_msgtype" ] = "logout";
    m_spotifyResolver.data()->sendMessage( msg );
    s_instance = 0;
}


void
SpotifyAccount::startPlaylistSync( SpotifyPlaylistInfo* playlist )
{
    if ( !playlist )
        return;

    QVariantMap msg;
    msg[ "playlistid" ] = playlist->plid;
    msg[ "sync" ] = true;

    if( playlist->loveSync )
    {
        msg[ "_msgtype" ] = "setSync";
        sendMessage( msg );
        if( playlist->sync && m_updaters.contains( playlist->plid ) )
            stopPlaylistSync(playlist, true);
    }
    else if( playlist->sync )
    {
        msg[ "_msgtype" ] = "getPlaylist";
        sendMessage( msg, this, "startPlaylistSyncWithPlaylist" );
    }

}


void
SpotifyAccount::startPlaylistSyncWithPlaylist( const QString& msgType, const QVariantMap& msg, const QVariant& )
{
    Q_UNUSED( msgType );
    tLog( LOGVERBOSE ) << Q_FUNC_INFO <<  "Got full spotify playlist body, creating a tomahawk playlist and enabling sync!!";
    const QString id = msg.value( "id" ).toString();
    const QString name = msg.value( "name" ).toString();
    const QString revid = msg.value( "revid" ).toString();
    const bool collaborative = msg.value( "collaborative" ).toBool();
    const bool owner = msg.value( "owner" ).toBool();

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
        //Q_ASSERT( m_updaters[ id ]->sync() == false ); /// Should have been unchecked/off before, but might not be if the user
        // changed spotify resolver meanwhile, so allow it for now
        SpotifyPlaylistInfo* info = m_allSpotifyPlaylists[ id ];
        if ( loveSync() && ( info && info->starContainer ) )
        {
            qDebug() << "Stopping playlist sync in favour for Love Sync";
            stopPlaylistSync( info, true );
        }
        else
        {
            m_updaters[ id ]->setSync( true );
//          m_updaters[ id ]->
            // TODO
        }
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
        updater->setOwner( owner );
        updater->setCollaborative( collaborative );
        m_updaters[ id ] = updater;
    }
}

void
SpotifyAccount::playlistCopyCreated( const QString& msgType, const QVariantMap& msg, const QVariant& )
{
    Q_UNUSED( msgType );

    qDebug() << Q_FUNC_INFO << "Got response from our createCopyPlaylist command, now creating updater and attaching";
    const bool success = msg.value( "success" ).toBool();

    if ( !success )
    {
        qWarning() << "Got FAILED return code from spotify resolver createPlaylist command, aborting sync";
        return;
    }

    const QString id = msg.value( "playlistid" ).toString();
    const QString revid = msg.value( "playlistid" ).toString();
    const QString qid = msg.value( "qid" ).toString();
    const QString title = msg.value( "playlistname" ).toString();

    qDebug() << msg;
    if ( !m_waitingForCreateReply.contains( qid ) )
    {
        qWarning() << "Got a createPlaylist reply for a playlist/qid we were not waiting for :-/ " << qid << m_waitingForCreateReply;
        return;
    }

    SpotifyPlaylistInfo *info = new SpotifyPlaylistInfo( title, id, revid, true, false, true );
    startPlaylistSync( info );
}


void
SpotifyAccount::playlistCreated( const QString& msgType, const QVariantMap& msg, const QVariant& )
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
    updater->setOwner( true );
    updater->setSync( true );
    m_updaters[ id ] = updater;
}


QString
SpotifyAccount::sendMessage( const QVariantMap &m, QObject* obj, const QString& slot, const QVariant& extraData )
{
    QVariantMap msg = m;
    const QString qid = uuid();

    if ( obj )
    {
        m_qidToSlotMap[ qid ] = qMakePair( obj, slot );
        msg[ "qid" ] = qid;

    }

    m_qidToExtraData[ qid ] = extraData;

    m_spotifyResolver.data()->sendMessage( msg );

    return qid;
}


void
SpotifyAccount::registerUpdaterForPlaylist( const QString& plId, SpotifyPlaylistUpdater* updater )
{
    m_updaters[ plId ] = updater;
}

void
SpotifyAccount::registerPlaylistInfo( const QString& name, const QString& plid, const QString &revid, const bool sync, const bool subscribed, const bool owner )
{
    m_allSpotifyPlaylists[ plid ] = new SpotifyPlaylistInfo( name, plid, revid, sync, subscribed, owner );
}

void
SpotifyAccount::registerPlaylistInfo( SpotifyPlaylistInfo* info )
{
    m_allSpotifyPlaylists[ info->plid ] = info;
}


void
SpotifyAccount::unregisterUpdater( const QString& plid )
{
    m_updaters.remove( plid );
}


void
SpotifyAccount::fetchFullPlaylist( SpotifyPlaylistInfo* playlist )
{
    Q_UNUSED( playlist );
}


bool
SpotifyAccount::deleteOnUnsync() const
{
    return configuration().value( "deleteOnUnsync", false ).toBool();
}


bool
SpotifyAccount::loveSync() const
{
    return configuration().value( "loveSync", false ).toBool();
}


void
SpotifyAccount::stopPlaylistSync( SpotifyPlaylistInfo* playlist, bool forceDontDelete )
{
    if ( !playlist )
        return;

    if ( loveSync() && playlist->starContainer )
    {
        qDebug() << "LoveSync in action, wont remove playlist " << playlist->name;
    }
    else if( !loveSync() )
    {
        QVariantMap msg;
        msg[ "_msgtype" ] = "removeFromSyncList";
        msg[ "playlistid" ] = playlist->plid;

        m_spotifyResolver.data()->sendMessage( msg );
    }

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
    SpotifyPlaylistInfo* info = m_allSpotifyPlaylists.value( spotifyPlaylistId, 0 );

    if ( info )
        info->sync = sync;


    if ( !m_configWidget.isNull() )
        m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists.values() );
}


void
SpotifyAccount::createActions()
{
    if ( !m_customActions.isEmpty() )
        return;

    QAction* syncAction = new QAction( 0 );
    syncAction->setIcon( QIcon( RESPATH "images/spotify-logo.png" ) );
    _detail::Closure* c = NewClosure( syncAction, SIGNAL( triggered( bool ) ), this, SLOT( syncActionTriggered( QAction* ) ), syncAction );
    c->setAutoDelete( false );
    ActionCollection::instance()->addAction( ActionCollection::LocalPlaylists, syncAction, this );
    syncAction->setData( Sync);
    m_customActions.append( syncAction );

    QAction* subscribeAction = new QAction( 0 );
    subscribeAction->setIcon( QIcon( RESPATH "images/spotify-logo.png" ) );
    c = NewClosure( subscribeAction, SIGNAL( triggered( bool ) ), this, SLOT( subscribeActionTriggered( QAction* ) ), subscribeAction );
    c->setAutoDelete( false );
    ActionCollection::instance()->addAction( ActionCollection::LocalPlaylists, subscribeAction, this );
    subscribeAction->setData( Subscribe );
    m_customActions.append( subscribeAction );

    QAction* collaborateAction = new QAction( 0 );
    collaborateAction->setIcon( QIcon( RESPATH "images/spotify-logo.png" ) );
    c = NewClosure( collaborateAction, SIGNAL( triggered( bool ) ), this, SLOT( collaborateActionTriggered( QAction* ) ), collaborateAction );
    c->setAutoDelete( false );
    ActionCollection::instance()->addAction( ActionCollection::LocalPlaylists, collaborateAction, this );
    collaborateAction->setData( Collaborate );
    m_customActions.append( collaborateAction );

}


void
SpotifyAccount::removeActions()
{
    foreach( QAction* action, m_customActions )
        ActionCollection::instance()->removeAction( action );

    m_customActions.clear();
}

