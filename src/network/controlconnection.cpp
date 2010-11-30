#include "controlconnection.h"

#include "tomahawk/tomahawkapp.h"
#include "remotecollection.h"
#include "filetransferconnection.h"
#include "database.h"
#include "databasecommand_collectionstats.h"
#include "dbsyncconnection.h"

#define TCP_TIMEOUT 600

using namespace Tomahawk;


ControlConnection::ControlConnection( Servent* parent )
    : Connection( parent )
    , m_dbsyncconn( 0 )
    , m_registered( false )
    , m_pingtimer( 0 )
{
    qDebug() << "CTOR controlconnection";
    setId("ControlConnection()");

    // auto delete when connection closes:
    connect( this, SIGNAL( finished() ), SLOT( deleteLater() ) );

    this->setMsgProcessorModeIn( MsgProcessor::UNCOMPRESS_ALL | MsgProcessor::PARSE_JSON );
    this->setMsgProcessorModeOut( MsgProcessor::COMPRESS_IF_LARGE );
}


ControlConnection::~ControlConnection()
{
    qDebug() << "DTOR controlconnection";

    delete m_pingtimer;
    m_servent->unregisterControlConnection(this);
    if( m_dbsyncconn ) m_dbsyncconn->deleteLater();
}


Connection*
ControlConnection::clone()
{
    ControlConnection* clone = new ControlConnection( servent() );
    clone->setOnceOnly( onceOnly() );
    clone->setName( name() );
    return clone;
}


void
ControlConnection::setup()
{
    qDebug() << Q_FUNC_INFO << id() << name();
    // setup source and remote collection for this peer
    m_source = source_ptr( new Source( id(), this ) );

    if( Servent::isIPWhitelisted( m_sock->peerAddress() ) )
    {
        // FIXME TODO blocking DNS lookup if LAN, slow/fails on windows?
        QHostInfo i = QHostInfo::fromName( m_sock->peerAddress().toString() );
        if( i.hostName().length() )
        {
            m_source->setFriendlyName( i.hostName() );
        }
    }
    else
    {
        m_source->setFriendlyName( QString( "%1" ).arg( name() ) );
    }

    // delay setting up collection/etc until source is synced.
    // we need it DB synced so it has an ID + exists in DB.
    connect( m_source.data(), SIGNAL( syncedWithDatabase() ),
                                SLOT( registerSource() ), Qt::QueuedConnection );

    m_source->doDBSync();

    m_pingtimer = new QTimer;
    m_pingtimer->setInterval( 5000 );
    connect( m_pingtimer, SIGNAL( timeout() ), SLOT( onPingTimer() ) );
    m_pingtimer->start();
    m_pingtimer_mark.start();
}


// source was synced to DB, set it up properly:
void
ControlConnection::registerSource()
{
    qDebug() << Q_FUNC_INFO;
    Source* source = (Source*) sender();
    Q_ASSERT( source == m_source.data() );
    // .. but we'll use the shared pointer we've already made:

    collection_ptr coll( new RemoteCollection( m_source ) );
    m_source->addCollection( coll );
    TomahawkApp::instance()->sourcelist().add( m_source );

    m_registered = true;
    setupDbSyncConnection();
    m_servent->registerControlConnection( this );
}


void
ControlConnection::setupDbSyncConnection( bool ondemand )
{
    if( m_dbsyncconn != NULL || ! m_registered )
        return;

    qDebug() << Q_FUNC_INFO << ondemand << m_source->id();
    Q_ASSERT( m_source->id() > 0 );

    if( ! m_dbconnkey.isEmpty() )
    {
        qDebug() << "Connecting to DBSync offer from peer...";
        m_dbsyncconn = new DBSyncConnection( m_servent, m_source );

        connect( m_dbsyncconn, SIGNAL( finished() ),
                 m_dbsyncconn,   SLOT( deleteLater() ) );

        connect( m_dbsyncconn, SIGNAL( destroyed( QObject* ) ),
                                 SLOT( dbSyncConnFinished( QObject* ) ), Qt::DirectConnection );

        m_servent->createParallelConnection( this, m_dbsyncconn, m_dbconnkey );
        m_dbconnkey.clear();
    }
    else if( !outbound() || ondemand ) // only one end makes the offer
    {
        qDebug() << "Offering a DBSync key to peer...";
        m_dbsyncconn = new DBSyncConnection( m_servent, m_source );

        connect( m_dbsyncconn, SIGNAL( finished() ),
                 m_dbsyncconn,   SLOT( deleteLater()) );

        connect( m_dbsyncconn, SIGNAL( destroyed(QObject* ) ),
                                 SLOT( dbSyncConnFinished( QObject* ) ), Qt::DirectConnection );

        QString key = uuid();
        m_servent->registerOffer( key, m_dbsyncconn );
        QVariantMap m;
        m.insert( "method", "dbsync-offer" );
        m.insert( "key", key );
        sendMsg( m );
    }
}


void
ControlConnection::dbSyncConnFinished( QObject* c )
{
    qDebug() << Q_FUNC_INFO << "DBSync connection closed (for now)";
    if( (DBSyncConnection*)c == m_dbsyncconn )
    {
        //qDebug() << "Setting m_dbsyncconn to NULL";
        m_dbsyncconn = NULL;
    }
}


DBSyncConnection*
ControlConnection::dbSyncConnection()
{
    qDebug() << Q_FUNC_INFO;
    if( m_dbsyncconn == NULL )
        setupDbSyncConnection( true );

    return m_dbsyncconn;
}


void
ControlConnection::handleMsg( msg_ptr msg )
{
    if ( msg->is( Msg::PING ) )
    {
        // qDebug() << "Received Connection PING, nice." << m_pingtimer_mark.elapsed();
        m_pingtimer_mark.restart();
        return;
    }

    // if small and not compresed, print it out for debug
    if( msg->length() < 1024 && !msg->is( Msg::COMPRESSED ) )
    {
        qDebug() << id() << "got msg:" << QString::fromAscii( msg->payload() );
    }

    // All control connection msgs are JSON
    if( !msg->is( Msg::JSON ) )
    {
        Q_ASSERT( msg->is( Msg::JSON ) );
        markAsFailed();
        return;
    }

    QVariantMap m = msg->json().toMap();
    if( !m.isEmpty() )
    {
        if( m.value("conntype").toString() == "request-offer" )
        {
            QString theirkey = m["key"].toString();
            QString ourkey   = m["offer"].toString();
            servent()->reverseOfferRequest( this, ourkey, theirkey );
        }
        else if( m.value( "method" ).toString() == "dbsync-offer" )
        {
            m_dbconnkey = m.value( "key" ).toString() ;
            setupDbSyncConnection();
        }
        else if( m.value( "method" ) == "protovercheckfail" )
        {
            qDebug() << "*** Remote peer protocol version mismatch, connection closed";
            shutdown( true );
            return;
        }
        else
        {
            qDebug() << id() << "Unhandled msg:" << QString::fromAscii( msg->payload() );
        }

        return;
    }

    qDebug() << id() << "Invalid msg:" << QString::fromAscii(msg->payload());
}



void
ControlConnection::onPingTimer()
{
    if ( m_pingtimer_mark.elapsed() >= TCP_TIMEOUT * 1000 )
    {
        qDebug() << "Timeout reached! Shutting down connection to" << m_source->friendlyName();
        shutdown( false );
    }

    sendMsg( Msg::factory( QByteArray(), Msg::PING ) );
}
