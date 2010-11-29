#include "filetransferconnection.h"

#include <QFile>

#include "tomahawk/tomahawkapp.h"
#include "tomahawk/result.h"

#include "bufferiodevice.h"
#include "controlconnection.h"
#include "databasecommand_loadfile.h"
#include "database.h"

// Msgs are framed, this is the size each msg we send containing audio data:
#define BLOCKSIZE 4096

using namespace Tomahawk;


FileTransferConnection::FileTransferConnection( Servent* s, ControlConnection* cc, QString fid, const Tomahawk::result_ptr& result )
    : Connection( s )
    , m_cc( cc )
    , m_fid( fid )
    , m_type( RECEIVING )
    , m_badded( 0 )
    , m_bsent( 0 )
    , m_allok( false )
    , m_result( result )
    , m_transferRate( 0 )
{
    qDebug() << Q_FUNC_INFO;

    BufferIODevice* bio = new BufferIODevice( result->size() );
    m_iodev = QSharedPointer<QIODevice>( bio ); // device audio data gets written to
    m_iodev->open( QIODevice::ReadWrite );

    APP->servent().registerFileTransferConnection( this );

    // if the audioengine closes the iodev (skip/stop/etc) then kill the connection
    // immediately to avoid unnecessary network transfer
    connect( m_iodev.data(), SIGNAL( aboutToClose() ), this, SLOT( shutdown() ), Qt::QueuedConnection );

    // auto delete when connection closes:
    connect( this, SIGNAL( finished() ), SLOT( deleteLater() ), Qt::QueuedConnection );

    // don't fuck with our messages at all. No compression, no parsing, nothing:
    this->setMsgProcessorModeIn ( MsgProcessor::NOTHING );
    this->setMsgProcessorModeOut( MsgProcessor::NOTHING );
}


FileTransferConnection::FileTransferConnection( Servent* s, ControlConnection* cc, QString fid )
    : Connection( s )
    , m_cc( cc )
    , m_fid( fid )
    , m_type( SENDING )
    , m_badded( 0 )
    , m_bsent( 0 )
    , m_allok( false )
    , m_transferRate( 0 )
{
    APP->servent().registerFileTransferConnection( this );
    // auto delete when connection closes:
    connect( this, SIGNAL( finished() ), SLOT( deleteLater() ), Qt::QueuedConnection );
}


FileTransferConnection::~FileTransferConnection()
{
    qDebug() << Q_FUNC_INFO << "TX/RX:" << bytesSent() << bytesReceived();
    if( m_type == RECEIVING && !m_allok )
    {
        qDebug() << "FTConnection closing before last data msg received, shame.";
        //TODO log the fact that our peer was bad-mannered enough to not finish the upload

        // protected, we could expose it:
        //m_iodev->setErrorString("FTConnection providing data went away mid-transfer");

        ((BufferIODevice*)m_iodev.data())->inputComplete();
    }

    APP->servent().onFileTransferFinished( this );
}


QString
FileTransferConnection::id() const
{
    return QString( "FTC[%1 %2]" )
              .arg( m_type == SENDING ? "TX" : "RX" )
              .arg( m_fid );
}


void
FileTransferConnection::showStats( qint64 tx, qint64 rx )
{
    if( tx > 0 || rx > 0 )
    {
        qDebug() << id()
                 << QString( "Down: %L1 bytes/sec," ).arg( rx )
                 << QString( "Up: %L1 bytes/sec" ).arg( tx );
    }

    m_transferRate = tx + rx;
    emit updated();
}


void
FileTransferConnection::setup()
{
    QList<source_ptr> sources = APP->sourcelist().sources();
    foreach( const source_ptr& src, sources )
    {
        // local src doesnt have a control connection, skip it:
        if( src.isNull() || src->isLocal() )
            continue;

        if ( src->controlConnection() == m_cc )
        {
            m_source = src;
            break;
        }
    }

    connect( this, SIGNAL( statsTick( qint64, qint64 ) ), SLOT( showStats( qint64, qint64 ) ) );
    if( m_type == RECEIVING )
    {
        qDebug() << "in RX mode";
        emit updated();
        return;
    }

    qDebug() << "in TX mode, fid:" << m_fid;

    DatabaseCommand_LoadFile* cmd = new DatabaseCommand_LoadFile( m_fid );
    connect( cmd, SIGNAL( result( Tomahawk::result_ptr ) ), SLOT( startSending( Tomahawk::result_ptr ) ) );
    TomahawkApp::instance()->database()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
FileTransferConnection::startSending( const Tomahawk::result_ptr& result )
{
    m_result = result;
    qDebug() << "Starting to transmit" << m_result->url();

    QSharedPointer<QIODevice> io = TomahawkApp::instance()->getIODeviceForUrl( m_result );
    if( !io )
    {
        qDebug() << "Couldn't read from source:" << m_result->url();
        shutdown();
        return;
    }

    m_readdev = QSharedPointer<QIODevice>( io );
    sendSome();

    emit updated();
}


void
FileTransferConnection::handleMsg( msg_ptr msg )
{
    Q_ASSERT( m_type == FileTransferConnection::RECEIVING );
    Q_ASSERT( msg->is( Msg::RAW ) );

    m_badded += msg->payload().length();
    ((BufferIODevice*)m_iodev.data())->addData( msg->payload() );

    //qDebug() << Q_FUNC_INFO << "flags" << (int) msg->flags()
    //         << "payload len" << msg->payload().length()
    //         << "written to device so far: " << m_badded;

    if( !msg->is( Msg::FRAGMENT ) )
    {
        qDebug() << "*** Got last msg in filetransfer. added" << m_badded
                 << "io size" << m_iodev->size();

        m_allok = true;
        // tell our iodev there is no more data to read, no args meaning a success:
        ((BufferIODevice*)m_iodev.data())->inputComplete();
        shutdown();
    }
}


Connection* FileTransferConnection::clone()
{
    Q_ASSERT( false );
    return 0;
}


void FileTransferConnection::sendSome()
{
    Q_ASSERT( m_type == FileTransferConnection::SENDING );

    QByteArray ba = m_readdev->read( BLOCKSIZE );
    m_bsent += ba.length();
    //qDebug() << "Sending" << ba.length() << "bytes of audiofile";

    if( m_readdev->atEnd() )
    {
        sendMsg( Msg::factory( ba, Msg::RAW ) );
        qDebug() << "Sent all. DONE." << m_bsent;
        shutdown( true );
        return;
    }
    else
    {
        // more to come -> FRAGMENT
        sendMsg( Msg::factory( ba, Msg::RAW | Msg::FRAGMENT ) );
    }

    // HINT: change the 0 to 50 to transmit at 640Kbps, for example
    //       (this is where upload throttling could be implemented)
    QTimer::singleShot( 0, this, SLOT( sendSome() ) );
}
