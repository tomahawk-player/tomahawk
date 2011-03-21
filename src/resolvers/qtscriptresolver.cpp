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
    , m_engine( new ScriptEngine( this ) )
    , m_thread( new QThread( this ) )
    , m_ready( false )
    , m_stopped( false )
{
    qDebug() << Q_FUNC_INFO << scriptPath;

    m_thread->start();

    QFile scriptFile( scriptPath );
    scriptFile.open( QIODevice::ReadOnly );
    m_engine->mainFrame()->setHtml( "<html><body></body></html>" );
    m_engine->mainFrame()->evaluateJavaScript( scriptFile.readAll() );
    scriptFile.close();

    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( "getSettings();" ).toMap();
    m_name       = m.value( "name" ).toString();
    m_weight     = m.value( "weight", 0 ).toUInt();
    m_timeout    = m.value( "timeout", 25 ).toUInt() * 1000;
    m_preference = m.value( "preference", 0 ).toUInt();

    qDebug() << "QTSCRIPT" << filePath() << "READY," << endl
    << "name" << m_name << endl
    << "weight" << m_weight << endl
    << "timeout" << m_timeout << endl
    << "preference" << m_preference;

    m_engine->moveToThread( m_thread );
    m_ready = true;
    Tomahawk::Pipeline::instance()->addResolver( this );
    
    connect( this, SIGNAL( destroyed( QObject* ) ), m_thread, SLOT( deleteLater() ) );
}


QtScriptResolver::~QtScriptResolver()
{
    Tomahawk::Pipeline::instance()->removeResolver( this );
    delete m_engine;
}


void
QtScriptResolver::resolve( const Tomahawk::query_ptr& query )
{
    QMetaObject::invokeMethod( m_engine, "resolve", Qt::QueuedConnection, Q_ARG( Tomahawk::query_ptr, query ) );
}


void
ScriptEngine::resolve( const Tomahawk::query_ptr& query )
{
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
        rp->setScore( m.value( "score" ).toFloat() * ( (float)m_parent->weight() / 100.0 ) );
        rp->setRID( uuid() );
        rp->setFriendlySource( m_parent->name() );

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
