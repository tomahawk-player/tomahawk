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
#include "utils/TomahawkUtils.h"
#include "ActionCollection.h"
#include "Pipeline.h"
#include "accounts/AccountManager.h"
#include "utils/Closure.h"

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

using namespace Tomahawk;
using namespace Accounts;


static QPixmap* s_icon = 0;

#ifdef Q_OS_MAC
static QString s_resolverId = "spotify-osx";
#elif defined(Q_OS_WIN)
static QString s_resolverId = "spotify-win";
#else
static QString s_resolverId = "spotify-linux";
#endif

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


SpotifyAccount::SpotifyAccount( const QString& accountId )
    : CustomAtticaAccount( accountId )
    , m_preventEnabling( false )
{
    init();
}


SpotifyAccount::~SpotifyAccount()
{
    clearUser();
}


void
SpotifyAccount::init()
{
    setAccountFriendlyName( "Spotify" );
    setAccountServiceName( "spotify" );

    if ( !AtticaManager::instance()->resolversLoaded() )
    {
        // If we're still waiting to load, wait for the attica resolvers to come down the pipe
        connect( AtticaManager::instance(), SIGNAL( resolversLoaded( Attica::Content::List ) ), this, SLOT( init() ), Qt::UniqueConnection );
        return;
    }

    qRegisterMetaType< Tomahawk::Accounts::SpotifyPlaylistInfo* >( "Tomahawk::Accounts::SpotifyPlaylist*" );

    AtticaManager::instance()->registerCustomAccount( s_resolverId, this );

    connect( AtticaManager::instance(), SIGNAL( resolverInstalled( QString ) ), this, SLOT( resolverInstalled( QString ) ) );

    const Attica::Content res = AtticaManager::instance()->resolverForId( s_resolverId );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );

    const QString path = configuration().value( "path" ).toString(); // Manual path override
    if ( !checkForResolver() && state != AtticaManager::Uninstalled )
    {
        // If the user manually deleted the resolver, mark it as uninstalled, so we re-fetch for the user
        AtticaManager::instance()->uninstallResolver( res );
    }
    else if ( state == AtticaManager::Installed || !path.isEmpty() )
    {
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
    m_spotifyResolver = QWeakPointer< ScriptResolver >( qobject_cast< ScriptResolver* >( Pipeline::instance()->addScriptResolver( path, enabled() ) ) );

    connect( m_spotifyResolver.data(), SIGNAL( changed() ), this, SLOT( resolverChanged() ) );
    connect( m_spotifyResolver.data(), SIGNAL( customMessage( QString,QVariantMap ) ), this, SLOT( resolverMessage( QString, QVariantMap ) ) );

    const bool hasMigrated = configuration().value( "hasMigrated" ).toBool();
    if ( !hasMigrated )
    {
        qDebug() << "Getting credentials from spotify resolver to migrate to in-app config";
        QVariantMap msg;
        msg[ "_msgtype" ] = "getCredentials";
        m_spotifyResolver.data()->sendMessage( msg );
    }

}


bool SpotifyAccount::checkForResolver()
{
#ifdef Q_OS_MAC
    const QDir path = QCoreApplication::applicationDirPath();
    QFile file( path.absoluteFilePath( "spotify_tomahawkresolver" ) );
    return file.exists();
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
    if ( m_spotifyResolver.isNull() && state == AtticaManager::Installed )
    {
        // We don;t have the resolver but it has been installed via attica already, so lets just turn it on
        hookupResolver();
    }
    else if ( m_spotifyResolver.isNull() )
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
        m_spotifyResolver.data()->start();
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


void
SpotifyAccount::resolverInstalled(const QString& resolverId)
{
    if ( resolverId == s_resolverId )
    {
        // We requested this install, so we want to launch it
        hookupResolver();
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

    m_preventEnabling = false;

    if ( !m_spotifyResolver.isNull() )
    {
        // replace
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
    QList<PlaylistUpdaterInterface*> updaters = playlist->updaters();
    foreach ( PlaylistUpdaterInterface* updater, updaters )
    {
        if ( SpotifyPlaylistUpdater* spotifyUpdater = qobject_cast< SpotifyPlaylistUpdater* >( updater ) )
        {
            if ( spotifyUpdater->sync() )
                found = true;
        }
    }

    if ( !found )
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

    SpotifyPlaylistUpdater* updater = 0;
    QList<PlaylistUpdaterInterface*> updaters = playlist->updaters();
    foreach ( PlaylistUpdaterInterface* u, updaters )
    {
        if ( SpotifyPlaylistUpdater* spotifyUpdater = qobject_cast< SpotifyPlaylistUpdater* >( u ) )
        {
            updater = spotifyUpdater;
        }
    }

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
        if ( info )
            info->sync = !updater->sync();

        if ( m_configWidget.data() )
            m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists );

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

        const QString newStartPos = msg.value( "newStartPosition" ).toString();
        const QVariantList tracksList = msg.value( "tracks" ).toList();
        const QString newRev = msg.value( "revid" ).toString();
        const QString oldRev = msg.value( "oldRev" ).toString();

        updater->spotifyTracksMoved( tracksList, newStartPos, newRev, oldRev  );
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
    else if( msgType == "spotifyError" )
    {
        const QString error = msg.value( "msg" ).toString();
        if( error.isEmpty() )
            return;

        if( msg.value( "isDebugMsg" ).toBool() )
            tDebug( LOGVERBOSE ) << "SpotifyResolverError: " << error;
        else
            JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( QString( "Spotify: %1" ).arg( error ) ) );
    }
    else if( msgType == "userChanged" )
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

        if ( success )
            createActions();

        configurationWidget(); // ensure it's created so we can set the login button
        if ( m_configWidget.data() )
        {
            const QString message = msg.value( "message" ).toString();
            m_configWidget.data()->loginResponse( success, message );
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

    qDeleteAll( m_allSpotifyPlaylists );
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
    if ( m_spotifyResolver.isNull() )
        return 0;

    if ( m_configWidget.isNull() )
    {
        m_configWidget = QWeakPointer< SpotifyAccountConfig >( new SpotifyAccountConfig( this ) );
        connect( m_configWidget.data(), SIGNAL( login( QString,QString ) ), this, SLOT( login( QString,QString ) ) );
        m_configWidget.data()->setPlaylists( m_allSpotifyPlaylists );
    }

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

    if ( !m_configWidget.data()->loggedInManually() && !m_configWidget.data()->username().isEmpty() && !m_configWidget.data()->password().isEmpty() )
    {
        // If the user never pressed log in, he might have just pressed ok or hit enter. So log in anyway
        login( m_configWidget.data()->username(), m_configWidget.data()->password() );
    }
}


void
SpotifyAccount::login( const QString& username, const QString& password )
{
    // Send the result to the resolver
    QVariantMap msg;
    msg[ "_msgtype" ] = "login";
    msg[ "username" ] = username;
    msg[ "password" ] = password;

    msg[ "highQuality" ] = m_configWidget.data()->highQuality();

    m_spotifyResolver.data()->sendMessage( msg );
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


void
SpotifyAccount::createActions()
{
    if ( !m_customActions.isEmpty() )
        return;

    QAction* action = new QAction( 0 );
    action->setIcon( QIcon( RESPATH "images/spotify-logo.png" ) );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( syncActionTriggered( bool ) ) );
    ActionCollection::instance()->addAction( ActionCollection::LocalPlaylists, action, this );
    m_customActions.append( action );
}


void
SpotifyAccount::removeActions()
{
    foreach( QAction* action, m_customActions )
        ActionCollection::instance()->removeAction( action );

    m_customActions.clear();
}

