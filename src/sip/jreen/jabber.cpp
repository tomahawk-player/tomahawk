#include "jabber.h"

#include "tomahawksettings.h"

#include <QtPlugin>
#include <QStringList>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>

JabberPlugin::JabberPlugin()
    : p( 0 )
    , m_menu( 0 )
    , m_addFriendAction( 0 )
{
}

JabberPlugin::~JabberPlugin()
{
    delete p;
}

void
JabberPlugin::setProxy( QNetworkProxy* proxy )
{
    p->setProxy( proxy );
}


const QString
JabberPlugin::name()
{
    return QString( MYNAME );
}

const QString
JabberPlugin::friendlyName()
{
    return QString( "Jabber" );
}

const QString
JabberPlugin::accountName()
{
    return TomahawkSettings::instance()->jabberUsername();
}

QMenu*
JabberPlugin::menu()
{
    return m_menu;
}

bool
JabberPlugin::connectPlugin( bool startup )
{
    qDebug() << Q_FUNC_INFO;

    if ( startup && !TomahawkSettings::instance()->jabberAutoConnect() )
        return false;

    QString jid       = TomahawkSettings::instance()->jabberUsername();
    QString server    = TomahawkSettings::instance()->jabberServer();
    QString password  = TomahawkSettings::instance()->jabberPassword();
    unsigned int port = TomahawkSettings::instance()->jabberPort();

    QStringList splitJid = jid.split( '@', QString::SkipEmptyParts );
    if ( splitJid.size() < 2 )
    {
        qDebug() << "JID did not have an @ in it, could not find a server part";
        return false;
    }

    if ( server.isEmpty() )
        server = splitJid[1];

    if ( port < 1 || port > 65535 || jid.isEmpty() || password.isEmpty() )
    {
        qDebug() << "Jabber credentials look wrong, not connecting";
        return false;
    }

    delete p;
    p = new Jabber_p( jid, password, server, port );

    QObject::connect( p, SIGNAL( peerOnline( QString ) ), SIGNAL( peerOnline( QString ) ) );
    QObject::connect( p, SIGNAL( peerOffline( QString ) ), SIGNAL( peerOffline( QString ) ) );
    QObject::connect( p, SIGNAL( msgReceived( QString, QString ) ), SIGNAL( msgReceived( QString, QString ) ) );

    QObject::connect( p, SIGNAL( connected() ), SIGNAL( onConnected() ) );
    QObject::connect( p, SIGNAL( disconnected() ), SIGNAL( onDisconnected() ) );

    return true;
}

void
JabberPlugin::disconnectPlugin()
{
    onDisconnected();
    
    if ( p )
        p->disconnect();

    delete p;
    p = 0;
}

void 
JabberPlugin::onConnected()
{
    if( !m_menu ) {
        m_menu = new QMenu( QString( "JREEN (" ).append( accountName() ).append(")" ) );
        m_addFriendAction = m_menu->addAction( "Add Friend..." );
        QAction *connectAction = m_menu->addAction( "Connect" );
        
        connect( m_addFriendAction, SIGNAL(triggered() ),
                this,              SLOT( showAddFriendDialog() ) );
        connect( connectAction, SIGNAL( triggered() ), SLOT( connectPlugin() ) );
        
        emit addMenu( m_menu );
    }
    
    emit connected();
}

void 
JabberPlugin::onDisconnected()
{
    if( m_menu && m_addFriendAction ) {
        emit removeMenu( m_menu );
        
        delete m_menu;
        m_menu = 0;
        m_addFriendAction = 0; // deleted by menu
    }
    
    emit disconnected();
}

void
JabberPlugin::sendMsg(const QString& to, const QString& msg)
{
    if ( p )
        p->sendMsg( to, msg );
}

void
JabberPlugin::broadcastMsg(const QString& msg)
{
    if ( p )
        p->broadcastMsg( msg );
}

void
JabberPlugin::addContact(const QString& jid, const QString& msg)
{
    if ( p )
        p->addContact( jid, msg );
}

void
JabberPlugin::showAddFriendDialog()
{
    bool ok;
    QString id = QInputDialog::getText( 0, tr( "Add Friend" ),
                                              tr( "Enter Jabber ID:" ), QLineEdit::Normal,
                                              "", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to add jabber contact to roster:" << id;
    addContact( id );
}

Q_EXPORT_PLUGIN2( sip, JabberPlugin )
