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

#include "scriptresolver.h"

#include <QtEndian>

#include "artist.h"
#include "album.h"
#include "pipeline.h"
#include "sourcelist.h"
#include "utils/tomahawkutils.h"


ScriptResolver::ScriptResolver( const QString& exe )
    : Tomahawk::ExternalResolver( exe )
    , m_num_restarts( 0 )
    , m_msgsize( 0 )
    , m_ready( false )
    , m_stopped( false )
{
    qDebug() << Q_FUNC_INFO << exe;
    connect( &m_proc, SIGNAL( readyReadStandardError() ), SLOT( readStderr() ) );
    connect( &m_proc, SIGNAL( readyReadStandardOutput() ), SLOT( readStdout() ) );
    connect( &m_proc, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( cmdExited( int, QProcess::ExitStatus ) ) );

    QString filepath = filePath();
#ifdef WIN32
    // have to enclose in quotes if path contains spaces on windows...
    filepath = QString( "\"%1\"" ).arg( filepath );
#endif

    m_proc.start( filepath );
}


ScriptResolver::~ScriptResolver()
{
    stop();

    Tomahawk::Pipeline::instance()->removeResolver( this );

    if( !m_configWidget.isNull() )
        delete m_configWidget.data();
}


void
ScriptResolver::readStderr()
{
    qDebug() << "SCRIPT_STDERR" << filePath() << m_proc.readAllStandardError();
}


void
ScriptResolver::readStdout()
{
//    qDebug() << Q_FUNC_INFO << m_proc.bytesAvailable();

    if( m_msgsize == 0 )
    {
        if( m_proc.bytesAvailable() < 4 ) return;
        quint32 len_nbo;
        m_proc.read( (char*) &len_nbo, 4 );
        m_msgsize = qFromBigEndian( len_nbo );
//        qDebug() << Q_FUNC_INFO << "msgsize" << m_msgsize;
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

        if( m_proc.bytesAvailable() )
            QTimer::singleShot( 0, this, SLOT( readStdout() ) );
    }
}


void
ScriptResolver::sendMsg( const QByteArray& msg )
{
//    qDebug() << Q_FUNC_INFO << m_ready << msg << msg.length();

    quint32 len;
    qToBigEndian( msg.length(), (uchar*) &len );
    m_proc.write( (const char*) &len, 4 );
    m_proc.write( msg );
}


void
ScriptResolver::handleMsg( const QByteArray& msg )
{
//    qDebug() << Q_FUNC_INFO << msg.size() << QString::fromAscii( msg );

    bool ok;
    QVariant v = m_parser.parse( msg, &ok );
    if( !ok || v.type() != QVariant::Map )
    {
        Q_ASSERT(false);
        return;
    }
    QVariantMap m = v.toMap();
    QString msgtype = m.value( "_msgtype" ).toString();

    if( msgtype == "settings" )
    {
        doSetup( m );
        return;
    }
    else if( msgtype == "confwidget" )
    {
        setupConfWidget( m );
        return;
    }

    if( msgtype == "results" )
    {
        const QString qid = m.value( "qid" ).toString();
        QList< Tomahawk::result_ptr > results;
        const QVariantList reslist = m.value( "results" ).toList();

        foreach( const QVariant& rv, reslist )
        {
            QVariantMap m = rv.toMap();
            qDebug() << "Found result:" << m;

            Tomahawk::result_ptr rp( new Tomahawk::Result() );
            Tomahawk::artist_ptr ap = Tomahawk::Artist::get( 0, m.value( "artist" ).toString() );
            rp->setArtist( ap );
            rp->setAlbum( Tomahawk::Album::get( 0, m.value( "album" ).toString(), ap ) );
            rp->setTrack( m.value( "track" ).toString() );
            rp->setDuration( m.value( "duration" ).toUInt() );
            rp->setBitrate( m.value( "bitrate" ).toUInt() );
            rp->setUrl( m.value( "url" ).toString() );
            rp->setSize( m.value( "size" ).toUInt() );
            rp->setScore( m.value( "score" ).toFloat() * ( (float)weight() / 100.0 ) );
            rp->setRID( uuid() );
            rp->setFriendlySource( m_name );

            rp->setMimetype( m.value( "mimetype" ).toString() );
            if ( rp->mimetype().isEmpty() )
            {
                rp->setMimetype( TomahawkUtils::extensionToMimetype( m.value( "extension" ).toString() ) );
                Q_ASSERT( !rp->mimetype().isEmpty() );
            }
            if ( m.contains( "year" ) )
            {
                QVariantMap attr;
                attr[ "releaseyear" ] = m.value( "year" );
                rp->setAttributes( attr );
            }

            results << rp;
        }

        Tomahawk::Pipeline::instance()->reportResults( qid, results );
    }
}


void
ScriptResolver::cmdExited( int code, QProcess::ExitStatus status )
{
    m_ready = false;
    qDebug() << Q_FUNC_INFO << "SCRIPT EXITED, code" << code << "status" << status << filePath();
    Tomahawk::Pipeline::instance()->removeResolver( this );

    if( m_stopped )
    {
        qDebug() << "*** Script resolver stopped ";
        emit finished();

        return;
    }

    if( m_num_restarts < 10 )
    {
        m_num_restarts++;
        qDebug() << "*** Restart num" << m_num_restarts;
        m_proc.start( filePath() );
    }
    else
    {
        qDebug() << "*** Reached max restarts, not restarting.";
    }
}


void
ScriptResolver::resolve( const Tomahawk::query_ptr& query )
{
    QVariantMap m;
    m.insert( "_msgtype", "rq" );

    if ( query->isFullTextQuery() )
    {
        m.insert( "fulltext", query->fullTextQuery() );
        m.insert( "artist", query->artist() );
        m.insert( "track", query->fullTextQuery() );
        m.insert( "qid", query->id() );
    }
    else
    {
        m.insert( "artist", query->artist() );
        m.insert( "track", query->track() );
        m.insert( "qid", query->id() );
    }

    const QByteArray msg = m_serializer.serialize( QVariant( m ) );
    sendMsg( msg );
}


void
ScriptResolver::doSetup( const QVariantMap& m )
{
//    qDebug() << Q_FUNC_INFO << m;

    m_name       = m.value( "name" ).toString();
    m_weight     = m.value( "weight", 0 ).toUInt();
    m_timeout    = m.value( "timeout", 25 ).toUInt() * 1000;
    qDebug() << "SCRIPT" << filePath() << "READY," << endl
             << "name" << m_name << endl
             << "weight" << m_weight << endl
             << "timeout" << m_timeout;

    m_ready = true;
    Tomahawk::Pipeline::instance()->addResolver( this );
}

void
ScriptResolver::setupConfWidget( const QVariantMap& m )
{
    bool compressed = m.value( "compressed", "false" ).toString() == "true";
    qDebug() << "Resolver has a preferences widget! compressed?" << compressed << m;

    QByteArray uiData = m[ "widget" ].toByteArray();
    if( compressed )
        uiData = qUncompress( QByteArray::fromBase64( uiData ) );
    else
        uiData = QByteArray::fromBase64( uiData );

    if( m.contains( "images" ) )
        uiData = fixDataImagePaths( uiData, compressed, m[ "images" ].toMap() );
    m_configWidget = QWeakPointer< QWidget >( widgetFromData( uiData, 0 ) );

    emit changed();
}


void
ScriptResolver::saveConfig()
{
    Q_ASSERT( !m_configWidget.isNull() );

    QVariantMap m;
    m.insert( "_msgtype", "setpref" );
    QVariant widgets = configMsgFromWidget( m_configWidget.data() );
    m.insert( "widgets", widgets );
    QByteArray data = m_serializer.serialize( m );
//     qDebug() << "Got widgets and data;" << widgets << data;
    sendMsg( data );
}


QWidget* ScriptResolver::configUI() const
{
    if( m_configWidget.isNull() )
        return 0;
    else
        return m_configWidget.data();
}


void
ScriptResolver::stop()
{
    m_stopped = true;
    qDebug() << "KILLING PROCESS!";
    m_proc.kill();
}
