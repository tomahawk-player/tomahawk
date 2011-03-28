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

#include "database/database.h"
#include "network/controlconnection.h"
#include "sourcelist.h"
#include "tomahawksettings.h"
#include "tomahawk/tomahawkapp.h"

#include "config.h"


SipHandler::SipHandler( QObject* parent )
    : QObject( parent )
    , m_connected( false )
{
    loadPlugins( findPlugins() );

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );
}


SipHandler::~SipHandler()
{
    disconnectPlugins();
}


QList< SipPlugin* >
SipHandler::plugins() const
{
    return m_plugins;
}


void
SipHandler::onSettingsChanged()
{
    disconnectPlugins();
    connectPlugins();
}


QStringList
SipHandler::findPlugins()
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
SipHandler::loadPlugins( const QStringList& paths )
{
    foreach ( QString fileName, paths )
    {
        if ( !QLibrary::isLibrary( fileName ) )
            continue;

        qDebug() << "Trying to load plugin:" << fileName;
        loadPlugin( fileName );
    }
}


void
SipHandler::loadPlugin( const QString& path )
{
    QPluginLoader loader( path );
    QObject* plugin = loader.instance();
    if ( !plugin )
    {
        qDebug() << "Error loading plugin:" << loader.errorString();
    }

    SipPlugin* sip = qobject_cast<SipPlugin*>(plugin);
    if ( sip )
    {
        if ( pluginLoaded( sip->name() ) )
        {
            qDebug() << "Plugin" << sip->name() << "already loaded! Not loading:" << loader.fileName();
            return;
        }
        qDebug() << "Loaded plugin:" << loader.fileName();

        QObject::connect( sip, SIGNAL( peerOnline( QString ) ), SLOT( onPeerOnline( QString ) ) );
        QObject::connect( sip, SIGNAL( peerOffline( QString ) ), SLOT( onPeerOffline( QString ) ) );
        QObject::connect( sip, SIGNAL( msgReceived( QString, QString ) ), SLOT( onMessage( QString, QString ) ) );

        QObject::connect( sip, SIGNAL( connected() ), SIGNAL( connected() ) );
        QObject::connect( sip, SIGNAL( disconnected() ), SIGNAL( disconnected() ) );
        QObject::connect( sip, SIGNAL( error( int, QString ) ), SLOT( onError( int, QString ) ) );

        m_plugins << sip;
    }
}


bool
SipHandler::pluginLoaded( const QString& name ) const
{
    foreach( SipPlugin* plugin, m_plugins )
    {
        if ( plugin->name() == name )
            return true;
    }

    return false;
}


void
SipHandler::connectPlugins( bool startup, const QString &pluginName )
{
#ifndef TOMAHAWK_HEADLESS
    if ( !TomahawkSettings::instance()->acceptedLegalWarning() )
    {
        int result = QMessageBox::question(
            TomahawkApp::instance()->mainWindow(), "Legal Warning",
            "By pressing OK below, you agree that your use of Tomahawk will be in accordance with any applicable laws, including copyright and intellectual property laws, in effect in your country of residence, and indemify the Tomahawk developers and project from liability should you choose to break those laws.\n\nFor more information, please see http://gettomahawk.com/legal",
            "I Do Not Agree", "I Agree"
        );
        if ( result != 1 )
            return;
        else
            TomahawkSettings::instance()->setAcceptedLegalWarning( true );
    }
#endif
    foreach( SipPlugin* sip, m_plugins )
    {
        if ( pluginName.isEmpty() || ( !pluginName.isEmpty() && sip->name() == pluginName ) )
            sip->connectPlugin( startup );
    }

    if ( pluginName.isEmpty() )
    {
        m_connected = true;
    }
}


void
SipHandler::disconnectPlugins( const QString &pluginName )
{
    foreach( SipPlugin* sip, m_plugins )
    {
        if ( pluginName.isEmpty() || ( !pluginName.isEmpty() && sip->name() == pluginName ) )
            sip->disconnectPlugin();
    }

    if ( pluginName.isEmpty() )
    {
        SourceList::instance()->removeAllRemote();
        m_connected = false;
    }
}


void
SipHandler::toggleConnect()
{
    if( m_connected )
        disconnectPlugins();
    else
        connectPlugins();
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
SipHandler::onMessage( const QString& from, const QString& msg )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "SIP Message:" << from << msg;

    QJson::Parser parser;
    bool ok;
    QVariant v = parser.parse( msg.toAscii(), &ok );
    if ( !ok  || v.type() != QVariant::Map )
    {
        qDebug() << "Invalid JSON in XMPP msg";
        return;
    }

    QVariantMap m = v.toMap();
    /*
      If only one party is externally visible, connection is obvious
      If both are, peer with lowest IP address initiates the connection.
      This avoids dupe connections.
     */
    if ( m.value( "visible" ).toBool() )
    {
        if( !Servent::instance()->visibleExternally() ||
            Servent::instance()->externalAddress() <= m.value( "ip" ).toString() )
        {
            qDebug() << "Initiate connection to" << from;
            Servent::instance()->connectToPeer( m.value( "ip" ).toString(),
                                          m.value( "port" ).toInt(),
                                          m.value( "key" ).toString(),
                                          from,
                                          m.value( "uniqname" ).toString() );
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
}


void
SipHandler::onError( int code, const QString& msg )
{
    qWarning() << "Failed to connect to SIP:" << code << msg;

    if ( code == SipPlugin::AuthError )
    {
        emit authError();
    }
    else
    {
        SipPlugin* sip = qobject_cast<SipPlugin*>(sender());
        QTimer::singleShot( 10000, sip, SLOT( connectPlugin() ) );
    }
}
