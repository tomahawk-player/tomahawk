#include "msgprocessor.h"

#include "network/servent.h"


MsgProcessor::MsgProcessor( quint32 mode, quint32 t ) :
    QObject(), m_mode( mode ), m_threshold( t ), m_totmsgsize( 0 )
{
    moveToThread( Servent::instance()->thread() );
}


void
MsgProcessor::append( msg_ptr msg )
{
    if( QThread::currentThread() != thread() )
    {
        qDebug() << "reinvoking msgprocessor::append in correct thread, ie not" << QThread::currentThread();
        QMetaObject::invokeMethod( this, "append", Qt::QueuedConnection, Q_ARG(msg_ptr, msg) );
        return;
    }

    m_msgs.append( msg );
    m_msg_ready.insert( msg.data(), false );

    m_totmsgsize += msg->payload().length();

    if( m_mode & NOTHING )
    {
        //qDebug() << "MsgProcessor::NOTHING";
        handleProcessedMsg( msg );
        return;
    }

    QFuture<msg_ptr> fut = QtConcurrent::run(&MsgProcessor::process, msg, m_mode, m_threshold);
    QFutureWatcher<msg_ptr> * watcher = new QFutureWatcher<msg_ptr>;
    connect( watcher, SIGNAL( finished() ),
             this, SLOT( processed() ),
             Qt::QueuedConnection );

    watcher->setFuture( fut );
}


void
MsgProcessor::processed()
{
    QFutureWatcher<msg_ptr> * watcher = (QFutureWatcher<msg_ptr> *) sender();
    msg_ptr msg = watcher->result();
    watcher->deleteLater();
    handleProcessedMsg( msg );
}


void
MsgProcessor::handleProcessedMsg( msg_ptr msg )
{
    Q_ASSERT( QThread::currentThread() == thread() );

    m_msg_ready.insert( msg.data(), true );

    while( !m_msgs.isEmpty() )
    {
        if( m_msg_ready.value( m_msgs.first().data() ) )
        {
            msg_ptr m = m_msgs.takeFirst();
            m_msg_ready.remove( m.data() );
            //qDebug() << Q_FUNC_INFO << "totmsgsize:" << m_totmsgsize;
            emit ready( m );
        }
        else
        {
            return;
        }
    }

    //qDebug() << Q_FUNC_INFO << "EMPTY, no msgs left.";
    emit empty();
}


/// This method is run by QtConcurrent:
msg_ptr
MsgProcessor::process( msg_ptr msg, quint32 mode, quint32 threshold )
{
    // uncompress if needed
    if( (mode & UNCOMPRESS_ALL) && msg->is( Msg::COMPRESSED ) )
    {
        qDebug() << "MsgProcessor::UNCOMPRESSING";
        msg->m_payload = qUncompress( msg->payload() );
        msg->m_length  = msg->m_payload.length();
        msg->m_flags ^= Msg::COMPRESSED;
    }

    // parse json payload into qvariant if needed
    if( (mode & PARSE_JSON) &&
        msg->is( Msg::JSON ) &&
        msg->m_json_parsed == false )
    {
        qDebug() << "MsgProcessor::PARSING JSON";
        bool ok;
        QJson::Parser parser;
        msg->m_json = parser.parse( msg->payload(), &ok );
        msg->m_json_parsed = true;
    }

    // compress if needed
    if( (mode & COMPRESS_IF_LARGE) &&
        !msg->is( Msg::COMPRESSED )
        && msg->length() > threshold )
    {
        qDebug() << "MsgProcessor::COMPRESSING";
        msg->m_payload = qCompress( msg->payload(), 9 );
        msg->m_length  = msg->m_payload.length();
        msg->m_flags |= Msg::COMPRESSED;
    }
    return msg;
}
