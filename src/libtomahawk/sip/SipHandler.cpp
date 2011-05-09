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

#include "SipHandler.h"
#include "sip/SipPlugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>
#include <QMessageBox>

#include "functimeout.h"

#include "database/database.h"
#include "network/controlconnection.h"
#include "sourcelist.h"
#include "tomahawksettings.h"
//#include "tomahawk/tomahawkapp.h"

#include "config.h"


//remove
#include <QLabel>

SipHandler* SipHandler::s_instance = 0;

SipHandler* SipHandler::instance()
{
    if( s_instance == 0 )
        s_instance = new SipHandler( 0 );
    return s_instance;
}

SipHandler::SipHandler( QObject* parent )
    : QObject( parent )
    , m_connected( false )
{
    s_instance = this;

    loadPluginFactories( findPluginFactories() );

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );
}


SipHandler::~SipHandler()
{
    disconnectAll();
}

const QPixmap SipHandler::avatar( const QString& name ) const
{
    qDebug() << Q_FUNC_INFO << "Getting avatar" << name << m_usernameAvatars.keys();
    if( m_usernameAvatars.keys().contains( name ) )
    {
        qDebug() << Q_FUNC_INFO << "Getting avatar and avatar != null ";
        Q_ASSERT(!m_usernameAvatars.value( name ).isNull());
        return m_usernameAvatars.value( name );
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "Getting avatar and avatar == null, GAAAAAH ";
        return QPixmap();
    }
}

const SipInfo
SipHandler::sipInfo(const QString& peerId) const
{
    return m_peersSipInfos.value( peerId );
}

void
SipHandler::onSettingsChanged()
{
    checkSettings();
}


QStringList
SipHandler::findPluginFactories()
{
    QStringList paths;
    QList< QDir > pluginDirs;

    QDir appDir( qApp->applicationDirPath() );
    #ifdef Q_OS_MAC
    if ( appDir.dirName() == "MacOS" )
    {
        // Development convenience-hack
        appDir.cdUp();
        appDir.cdUp();
        appDir.cdUp();
    }
    #endif

    QDir libDir( CMAKE_INSTALL_PREFIX "/lib" );

    QDir lib64Dir( appDir );
    lib64Dir.cdUp();
    lib64Dir.cd( "lib64" );

    pluginDirs << appDir << libDir << lib64Dir << QDir( qApp->applicationDirPath() );
    foreach ( const QDir& pluginDir, pluginDirs )
    {
        qDebug() << "Checking directory for plugins:" << pluginDir;
        foreach ( QString fileName, pluginDir.entryList( QStringList() << "*tomahawk_sip*.so" << "*tomahawk_sip*.dylib" << "*tomahawk_sip*.dll", QDir::Files ) )
        {
            if ( fileName.startsWith( "libtomahawk_sip" ) )
            {
                const QString path = pluginDir.absoluteFilePath( fileName );
                if ( !paths.contains( path ) )
                    paths << path;
            }
        }
    }

    return paths;
}


void
SipHandler::loadPluginFactories( const QStringList& paths )
{
    foreach ( QString fileName, paths )
    {
        if ( !QLibrary::isLibrary( fileName ) )
            continue;

        qDebug() << "Trying to load plugin:" << fileName;
        loadPluginFactory( fileName );
    }
}

SipPlugin*
SipHandler::createPlugin( const QString& factoryId )
{
    Q_ASSERT( m_pluginFactories.contains( factoryId ) );

    SipPlugin* sip = m_pluginFactories[ factoryId ]->createPlugin();
    hookUpPlugin( sip );

    emit pluginAdded( sip );
    return sip;
}

SipPlugin*
SipHandler::loadPlugin( const QString& pluginId )
{
    QString factoryName = factoryFromId( pluginId );

    Q_ASSERT( m_pluginFactories.contains( factoryName ) );

    SipPlugin* sip = m_pluginFactories[ factoryName ]->createPlugin( pluginId );

    // caller responsible for calling pluginAdded() and hookupPlugin
    return sip;
}

void
SipHandler::removePlugin( SipPlugin* p )
{
    p->disconnectPlugin();

    m_allPlugins.removeAll( p );
    m_enabledPlugins.removeAll( p );

    TomahawkSettings::instance()->removeSipPlugin( p->pluginId() );

    emit pluginRemoved( p );
}


void
SipHandler::hookUpPlugin( SipPlugin* sip )
{
    QObject::connect( sip, SIGNAL( peerOnline( QString ) ), SLOT( onPeerOnline( QString ) ) );
    QObject::connect( sip, SIGNAL( peerOffline( QString ) ), SLOT( onPeerOffline( QString ) ) );
    QObject::connect( sip, SIGNAL( msgReceived( QString, QString ) ), SLOT( onMessage( QString, QString ) ) );
    QObject::connect( sip, SIGNAL( sipInfoReceived( QString, SipInfo ) ), SLOT( onSipInfo( QString, SipInfo ) ) );

    QObject::connect( sip, SIGNAL( error( int, QString ) ), SLOT( onError( int, QString ) ) );
    QObject::connect( sip, SIGNAL( stateChanged( SipPlugin::ConnectionState ) ), SLOT( onStateChanged( SipPlugin::ConnectionState ) ) );

    QObject::connect( sip, SIGNAL( avatarReceived( QString, QPixmap ) ), SLOT( onAvatarReceived( QString, QPixmap ) ) );
    QObject::connect( sip, SIGNAL( avatarReceived( QPixmap ) ), SLOT( onAvatarReceived( QPixmap ) ) );
}


void
SipHandler::loadPluginFactory( const QString& path )
{
    QPluginLoader loader( path );
    QObject* plugin = loader.instance();
    if ( !plugin )
    {
        qDebug() << "Error loading plugin:" << loader.errorString();
    }

    SipPluginFactory* sipfactory = qobject_cast<SipPluginFactory*>(plugin);
    if ( sipfactory )
    {
        qDebug() << "Loaded plugin factory:" << loader.fileName() << sipfactory->factoryId() << sipfactory->prettyName();
        m_pluginFactories[ sipfactory->factoryId() ] = sipfactory;
    } else
    {
        qDebug() << "Loaded invalid plugin.." << loader.fileName();
    }
}


bool
SipHandler::pluginLoaded( const QString& pluginId ) const
{
    foreach( SipPlugin* plugin, m_allPlugins )
    {
        if ( plugin->pluginId() == pluginId )
            return true;
    }

    return false;
}


void
SipHandler::checkSettings()
{
    foreach( SipPlugin* sip, m_allPlugins )
    {
        sip->checkSettings();
    }
}

void
SipHandler::addSipPlugin( SipPlugin* p, bool enabled, bool startup )
{
    m_allPlugins << p;

    hookUpPlugin( p );
    if ( enabled )
    {
        p->connectPlugin( startup );
        m_enabledPlugins << p;
    }

    emit pluginAdded( p );
}

void
SipHandler::removeSipPlugin( SipPlugin* p )
{
    p->disconnectPlugin();
    emit pluginRemoved( p );
    // emit first so sipmodel can find the indexOf

    TomahawkSettings::instance()->removeSipPlugin( p->pluginId() );
    m_allPlugins.removeAll( p );
    m_enabledPlugins.removeAll( p );
}

bool
SipHandler::hasPluginType( const QString& factoryId ) const
{
    foreach( SipPlugin* p, m_allPlugins ) {
        if( factoryFromId( p->pluginId() ) == factoryId )
            return true;
    }
    return false;
}


void
SipHandler::loadFromConfig( bool startup )
{
    QStringList pluginIds = TomahawkSettings::instance()->sipPlugins();
    QStringList enabled = TomahawkSettings::instance()->enabledSipPlugins();
    foreach( const QString& pluginId, pluginIds )
    {
        QString pluginFactory = factoryFromId( pluginId );
        if( m_pluginFactories.contains( pluginFactory ) )
        {
            SipPlugin* p = loadPlugin( pluginId );
            addSipPlugin( p, enabled.contains( pluginId ), startup );
        }
    }
    m_connected = true;
}

void
SipHandler::connectAll()
{
    foreach( SipPlugin* sip, m_enabledPlugins )
    {
        sip->connectPlugin();
    }
    m_connected = true;
}


void
SipHandler::disconnectAll()
{
    foreach( SipPlugin* p, m_connectedPlugins )
        p->disconnectPlugin();

    SourceList::instance()->removeAllRemote();
    m_connected = false;
}

void
SipHandler::disablePlugin( SipPlugin* p )
{
    Q_ASSERT( m_enabledPlugins.contains( p ) );

    TomahawkSettings::instance()->disableSipPlugin( p->pluginId() );
    p->disconnectPlugin();

    m_enabledPlugins.removeAll( p );
}

void
SipHandler::enablePlugin( SipPlugin* p )
{
    Q_ASSERT( !m_enabledPlugins.contains( p ) );
    p->connectPlugin();

    TomahawkSettings::instance()->enableSipPlugin( p->pluginId() );
    m_enabledPlugins << p;
}


void
SipHandler::connectPlugin( bool startup, const QString &pluginId )
{
#ifndef TOMAHAWK_HEADLESS
    if ( !TomahawkSettings::instance()->acceptedLegalWarning() )
    {
        int result = QMessageBox::question(
            //TomahawkApp::instance()->mainWindow(),
            0, tr( "Legal Warning" ),
            tr( "By pressing OK below, you agree that your use of Tomahawk will be in accordance with any applicable laws, including copyright and intellectual property laws, in effect in your country of residence, and indemnify the Tomahawk developers and project from liability should you choose to break those laws.\n\nFor more information, please see http://gettomahawk.com/legal" ),
            tr( "I Do Not Agree" ), tr( "I Agree" )
        );
        if ( result != 1 )
            return;
        else
            TomahawkSettings::instance()->setAcceptedLegalWarning( true );
    }
#endif
    foreach( SipPlugin* sip, m_allPlugins )
    {
        if ( sip->pluginId() == pluginId )
        {
            Q_ASSERT( m_enabledPlugins.contains( sip ) ); // make sure the plugin we're connecting is enabled. should always be the case
            sip->setProxy( m_proxy );
            sip->connectPlugin( startup );
        }
    }
}


void
SipHandler::disconnectPlugin( const QString &pluginName )
{
    foreach( SipPlugin* sip, m_connectedPlugins )
    {
        if ( sip->name() == pluginName )
            sip->disconnectPlugin();
    }
}

QList< SipPlugin* >
SipHandler::allPlugins() const
{
    return m_allPlugins;
}

QList< SipPlugin* >
SipHandler::enabledPlugins() const
{
    return m_enabledPlugins;
}

QList< SipPlugin* >
SipHandler::connectedPlugins() const
{
    return m_connectedPlugins;
}

QList< SipPluginFactory* >
SipHandler::pluginFactories() const
{
    return m_pluginFactories.values();
}


void
SipHandler::toggleConnect()
{
    if( m_connected )
        disconnectAll();
    else
        connectAll();
}

void
SipHandler::setProxy( const QNetworkProxy& proxy )
{
    qDebug() << Q_FUNC_INFO;

    m_proxy = proxy;

    foreach( SipPlugin* sip, m_allPlugins )
        sip->setProxy( proxy );
}


void
SipHandler::onPeerOnline( const QString& jid )
{
//    qDebug() << Q_FUNC_INFO;
    qDebug() << "SIP online:" << jid;

    SipPlugin* sip = qobject_cast<SipPlugin*>(sender());

    QVariantMap m;
    if( Servent::instance()->visibleExternally() )
    {
        QString key = uuid();
        ControlConnection* conn = new ControlConnection( Servent::instance() );

        const QString& nodeid = Database::instance()->dbid();

        //TODO: this is a terrible assumption, help me clean this up, mighty muesli!
        if ( jid.contains( "@conference.") )
            conn->setName( jid );
        else
            conn->setName( jid.left( jid.indexOf( "/" ) ) );

        conn->setId( nodeid );

        Servent::instance()->registerOffer( key, conn );
        m["visible"] = true;
        m["ip"] = Servent::instance()->externalAddress();
        m["port"] = Servent::instance()->externalPort();
        m["key"] = key;
        m["uniqname"] = nodeid;

        qDebug() << "Asking them to connect to us:" << m;
    }
    else
    {
        m["visible"] = false;
        qDebug() << "We are not visible externally:" << m;
    }

    QJson::Serializer ser;
    QByteArray ba = ser.serialize( m );

    sip->sendMsg( jid, QString::fromAscii( ba ) );
}


void
SipHandler::onPeerOffline( const QString& jid )
{
//    qDebug() << Q_FUNC_INFO;
    qDebug() << "SIP offline:" << jid;
}

void
SipHandler::onSipInfo( const QString& peerId, const SipInfo& info )
{
    qDebug() << Q_FUNC_INFO << "SIP Message:" << peerId << info;

    /*
      If only one party is externally visible, connection is obvious
      If both are, peer with lowest IP address initiates the connection.
      This avoids dupe connections.
     */
    if ( info.isVisible() )
    {
        if( !Servent::instance()->visibleExternally() ||
            Servent::instance()->externalAddress() <= info.host().hostName() )
        {
            qDebug() << "Initiate connection to" << peerId;
            Servent::instance()->connectToPeer( info.host().hostName(),
                                          info.port(),
                                          info.key(),
                                          peerId,
                                          info.uniqname() );
        }
        else
        {
            qDebug() << Q_FUNC_INFO << "They should be conecting to us...";
        }
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "They are not visible, doing nothing atm";
    }

    m_peersSipInfos.insert( peerId, info );
}

void
SipHandler::onMessage( const QString& from, const QString& msg )
{
    qDebug() << Q_FUNC_INFO;
}


void
SipHandler::onError( int code, const QString& msg )
{
    SipPlugin* sip = qobject_cast< SipPlugin* >( sender() );
    Q_ASSERT( sip );

    qWarning() << "Failed to connect to SIP:" << sip->accountName() << code << msg;

    if ( code == SipPlugin::AuthError )
    {
        emit authError( sip );
    }
    else
    {
        QTimer::singleShot( 10000, sip, SLOT( connectPlugin() ) );
    }
}

void
SipHandler::onStateChanged( SipPlugin::ConnectionState state )
{
    SipPlugin* sip = qobject_cast< SipPlugin* >( sender() );
    Q_ASSERT( sip );

    if ( sip->connectionState() == SipPlugin::Disconnected )
    {
        m_connectedPlugins.removeAll( sip );
        emit disconnected( sip );
    } else if ( sip->connectionState() == SipPlugin::Connected )
    {
        m_connectedPlugins.removeAll( sip );
        emit disconnected( sip );
    }

    emit stateChanged( sip, state );
}


void
SipHandler::onAvatarReceived( const QString& from, const QPixmap& avatar )
{
    qDebug() << Q_FUNC_INFO << "Set avatar on source for " << from;
    Q_ASSERT(!avatar.isNull());

    m_usernameAvatars.insert( from, avatar );

    //

    //Tomahawk::source_ptr source = ->source();
    ControlConnection *conn = Servent::instance()->lookupControlConnection( from );
    if( conn )
    {
        qDebug() << Q_FUNC_INFO << from << "got control connection";
        Tomahawk::source_ptr source = conn->source();
        if( source )
        {

            qDebug() << Q_FUNC_INFO << from << "got source, setting avatar";
            source->setAvatar( avatar );
        }
        else
        {
            qDebug() << Q_FUNC_INFO << from << "no source found, not setting avatar";
        }
    }
    else
    {
        qDebug() << Q_FUNC_INFO << from << "no control connection setup yet";
    }
}

void
SipHandler::onAvatarReceived( const QPixmap& avatar )
{
    qDebug() << Q_FUNC_INFO << "Set own avatar on MyCollection";
    SourceList::instance()->getLocal()->setAvatar( avatar );
}


QString
SipHandler::factoryFromId( const QString& pluginId ) const
{
    return pluginId.split( "_" ).first();
}

SipPluginFactory*
SipHandler::factoryFromPlugin( SipPlugin* p ) const
{
    QString factoryId = factoryFromId( p->pluginId() );
    return m_pluginFactories.value( factoryId, 0 );
}
