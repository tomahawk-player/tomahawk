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

#include "qtscriptresolver.h"

#include "artist.h"
#include "album.h"
#include "pipeline.h"
#include "sourcelist.h"
#include "utils/tomahawkutils.h"


QtScriptResolver::QtScriptResolver( const QString& scriptPath )
    : Tomahawk::ExternalResolver( scriptPath )
    , m_ready( false )
    , m_stopped( false )
{
    qDebug() << Q_FUNC_INFO << scriptPath;

    m_thread = new ScriptThread( scriptPath, this );
    connect( m_thread, SIGNAL( engineFound( QString, unsigned int, unsigned int, unsigned int ) ),
                         SLOT( onEngineFound( QString, unsigned int, unsigned int, unsigned int ) ) );

    m_thread->start();

    connect( this, SIGNAL( destroyed( QObject* ) ), m_thread, SLOT( deleteLater() ) );
}


QtScriptResolver::~QtScriptResolver()
{
    Tomahawk::Pipeline::instance()->removeResolver( this );
    delete m_thread;
}


void
QtScriptResolver::resolve( const Tomahawk::query_ptr& query )
{
    m_thread->resolve( query );
}


void
QtScriptResolver::onEngineFound( const QString& name, unsigned int weight, unsigned int timeout, unsigned int preference )
{
    m_name = name;
    m_weight = weight;
    m_timeout = timeout;
    m_preference = preference;

    qDebug() << "QTSCRIPT" << filePath() << "READY," << endl
        << "name" << m_name << endl
        << "weight" << m_weight << endl
        << "timeout" << m_timeout << endl
        << "preference" << m_preference;

    m_ready = true;
    Tomahawk::Pipeline::instance()->addResolver( this );
}


ScriptThread::ScriptThread( const QString& scriptPath, QtScriptResolver* parent )
    : QThread()
    , m_parent( parent )
    , m_scriptPath( scriptPath )
{
    moveToThread( this );
}


void
ScriptThread::resolve( const Tomahawk::query_ptr& query )
{
    m_engine->resolve( query );
}


void
ScriptThread::run()
{
    QTimer::singleShot( 0, this, SLOT( initEngine() ) );
    exec();
}


void
ScriptThread::initEngine()
{
    m_engine = new ScriptEngine( m_parent, this );
    QFile scriptFile( m_scriptPath );
    if ( !scriptFile.open( QIODevice::ReadOnly ) )
    {
        qDebug() << Q_FUNC_INFO << "Failed loading JavaScript resolver:" << m_scriptPath;
        deleteLater();
        return;
    }

    m_engine->mainFrame()->setHtml( "<html><body></body></html>" );
    m_engine->mainFrame()->evaluateJavaScript( scriptFile.readAll() );
    scriptFile.close();

    QString name;
    unsigned int weight, preference, timeout;
    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( "getSettings();" ).toMap();
    name       = m.value( "name" ).toString();
    weight     = m.value( "weight", 0 ).toUInt();
    timeout    = m.value( "timeout", 25 ).toUInt() * 1000;
    preference = m.value( "preference", 0 ).toUInt();

    qDebug() << Q_FUNC_INFO << name << weight << timeout << preference;
    emit engineFound( name, weight, timeout, preference );
}


void
ScriptEngine::resolve( const Tomahawk::query_ptr& query )
{
    if ( QThread::currentThread() != thread() )
    {
//        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "resolve",
                                   Qt::QueuedConnection,
                                   Q_ARG(Tomahawk::query_ptr, query)
        );
        return;
    }

    qDebug() << Q_FUNC_INFO << query->toString();
    QString eval = QString( "resolve( '%1', '%2', '%3', '%4' );" )
                      .arg( query->id().replace( "'", "\\'" ) )
                      .arg( query->artist().replace( "'", "\\'" ) )
                      .arg( query->album().replace( "'", "\\'" ) )
                      .arg( query->track().replace( "'", "\\'" ) );

    QList< Tomahawk::result_ptr > results;

    QVariantMap m = mainFrame()->evaluateJavaScript( eval ).toMap();
    qDebug() << "JavaScript Result:" << m;

    const QString qid = query->id();
    const QVariantList reslist = m.value( "results" ).toList();

    foreach( const QVariant& rv, reslist )
    {
        QVariantMap m = rv.toMap();
        qDebug() << "RES" << m;

        Tomahawk::result_ptr rp( new Tomahawk::Result() );
        Tomahawk::artist_ptr ap = Tomahawk::Artist::get( 0, m.value( "artist" ).toString() );
        rp->setArtist( ap );
        rp->setAlbum( Tomahawk::Album::get( 0, m.value( "album" ).toString(), ap ) );
        rp->setTrack( m.value( "track" ).toString() );
        rp->setBitrate( m.value( "bitrate" ).toUInt() );
        rp->setUrl( m.value( "url" ).toString() );
        rp->setSize( m.value( "size" ).toUInt() );
        rp->setScore( m.value( "score" ).toFloat() * ( (float)m_resolver->weight() / 100.0 ) );
        rp->setRID( uuid() );
        rp->setFriendlySource( m_resolver->name() );

        if ( m.contains( "year" ) )
        {
            QVariantMap attr;
            attr[ "releaseyear" ] = m.value( "year" );
            rp->setAttributes( attr );
        }

        rp->setDuration( m.value( "duration", 0 ).toUInt() );
        if ( rp->duration() <= 0 && m.contains( "durationString" ) )
        {
            QTime time = QTime::fromString( m.value( "durationString" ).toString(), "hh:mm:ss" );
            rp->setDuration( time.secsTo( QTime( 0, 0 ) ) * -1 );
        }

        rp->setMimetype( m.value( "mimetype" ).toString() );
        if ( rp->mimetype().isEmpty() )
        {
            rp->setMimetype( TomahawkUtils::extensionToMimetype( m.value( "extension" ).toString() ) );
            Q_ASSERT( !rp->mimetype().isEmpty() );
        }

        results << rp;
    }

    Tomahawk::Pipeline::instance()->reportResults( qid, results );
}


void
QtScriptResolver::stop()
{
    m_stopped = true;
    emit finished();
}
