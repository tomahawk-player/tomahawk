#include "scriptresolver.h"

#include <QtEndian>

#include "pipeline.h"
#include "sourcelist.h"


ScriptResolver::ScriptResolver(const QString& exe) :
    Tomahawk::Resolver()
    , m_cmd( exe )
    , m_num_restarts( 0 )
    , m_msgsize( 0 )
    , m_ready( false )
{
    qDebug() << Q_FUNC_INFO << exe;
    connect( &m_proc, SIGNAL(readyReadStandardError()), SLOT(readStderr()) );
    connect( &m_proc, SIGNAL(readyReadStandardOutput()), SLOT(readStdout()) );
    connect( &m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(cmdExited(int,QProcess::ExitStatus)) );

    m_proc.start( m_cmd );
}


void ScriptResolver::readStderr()
{
    qDebug() << "SCRIPT_STDERR" << m_cmd << m_proc.readAllStandardError();
}


void ScriptResolver::readStdout()
{
    qDebug() << Q_FUNC_INFO << m_proc.bytesAvailable();
    if( m_msgsize == 0 )
    {
        if( m_proc.bytesAvailable() < 4 ) return;
        quint32 len_nbo;
        m_proc.read( (char*) &len_nbo, 4 );
        m_msgsize = qFromBigEndian( len_nbo );
        qDebug() << Q_FUNC_INFO << "msgsize" << m_msgsize;
    }

    if( m_msgsize > 0 )
    {
        m_msg.append( m_proc.read( m_msgsize - m_msg.length() ) );
    }

    if( m_msgsize == (quint32) m_msg.length() )
    {
        handleMsg( m_msg );
        m_msgsize = 0;
        m_msg.clear();
        if( m_proc.bytesAvailable() ) QTimer::singleShot( 0, this, SLOT(readStdout()) );
    }
}


void ScriptResolver::sendMsg( const QByteArray& msg )
{
    qDebug() << Q_FUNC_INFO << m_ready << msg;

    if( !m_ready ) return;

    quint32 len;
    qToBigEndian( msg.length(), (uchar*) &len );
    m_proc.write( (const char*) &len, 4 );
    m_proc.write( msg );
}


void ScriptResolver::handleMsg( const QByteArray& msg )
{
    qDebug() << Q_FUNC_INFO << msg.size() << QString::fromAscii(msg);
    bool ok;
    QVariant v = m_parser.parse( msg, &ok );
    if( !ok || v.type() != QVariant::Map )
    {
        Q_ASSERT(false);
        return;
    }
    QVariantMap m = v.toMap();
    const QString& msgtype = m.value( "_msgtype" ).toString();

    if( msgtype == "settings" )
    {
        doSetup( m );
        return;
    }

    if( msgtype == "results" )
    {
        QList< Tomahawk::result_ptr > results;
        const QString& qid = m.value( "qid" ).toString();
        const QVariantList& reslist = m.value( "results" ).toList();
        Tomahawk::collection_ptr coll = SourceList::instance()->getLocal()->collection();
        foreach( const QVariant& rv, reslist )
        {
            qDebug() << "RES" << rv;
            Tomahawk::result_ptr rp( new Tomahawk::Result( rv, coll ) );
            results << rp;
        }
        Tomahawk::Pipeline::instance()->reportResults( qid, results );
    }
}


void ScriptResolver::cmdExited(int code, QProcess::ExitStatus status)
{
    m_ready = false;
    qDebug() << Q_FUNC_INFO << "SCRIPT EXITED, code" << code << "status" << status << m_cmd;
    Tomahawk::Pipeline::instance()->removeResolver( this );

    if( m_num_restarts < 10 )
    {
        m_num_restarts++;
        qDebug() << "*** Restart num" << m_num_restarts;
        m_proc.start( m_cmd );
    }
    else
    {
        qDebug() << "*** Reached max restarts, not restarting.";
    }
}


void ScriptResolver::resolve( const QVariant& v )
{
    QVariantMap m = v.toMap();
    m.insert( "_msgtype", "rq" );
    const QByteArray msg = m_serializer.serialize( m );
    qDebug() << "ASKING SCRIPT RESOLVER TO RESOLVE:" << msg;
    sendMsg( msg );
}


void ScriptResolver::doSetup( const QVariantMap& m )
{
    qDebug() << Q_FUNC_INFO << m;
    m_name       = m.value( "name" ).toString();
    m_weight     = m.value( "weight", 0 ).toUInt();
    m_timeout    = m.value( "timeout", 5000 ).toUInt();
    m_preference = m.value( "preference", 0 ).toUInt();
    qDebug() << "SCRIPT" << m_cmd << "READY, " << endl
             << " name" << m_name << endl
             << " weight" << m_weight << endl
             << " timeout" << m_timeout << endl
             << " preference" << m_preference
             ;
    m_ready = true;
    Tomahawk::Pipeline::instance()->addResolver( this );
}
