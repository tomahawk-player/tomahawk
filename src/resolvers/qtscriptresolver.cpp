#include "qtscriptresolver.h"

#include "artist.h"
#include "album.h"
#include "pipeline.h"
#include "sourcelist.h"
#include "utils/tomahawkutils.h"


QtScriptResolver::QtScriptResolver( const QString& scriptPath )
    : Tomahawk::ExternalResolver( scriptPath )
    , m_engine( new ScriptEngine( this ) )
    , m_ready( false )
    , m_stopped( false )
{
    qDebug() << Q_FUNC_INFO << scriptPath;

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

    m_ready = true;
    Tomahawk::Pipeline::instance()->addResolver( this );
}


QtScriptResolver::~QtScriptResolver()
{
    Tomahawk::Pipeline::instance()->removeResolver( this );
}


void
QtScriptResolver::resolve( const Tomahawk::query_ptr& query )
{
    qDebug() << Q_FUNC_INFO << query->toString();
    QString eval = QString( "resolve( '%1', '%2', '%3', '%4' );" )
                      .arg( query->id().replace( "'", "\\'" ) )
                      .arg( query->artist().replace( "'", "\\'" ) )
                      .arg( query->album().replace( "'", "\\'" ) )
                      .arg( query->track().replace( "'", "\\'" ) );

    QList< Tomahawk::result_ptr > results;

    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( eval ).toMap();
    qDebug() << "JavaScript Result:" << m;

    const QString qid = m.value( "qid" ).toString();
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

        results << rp;
    }

    Tomahawk::Pipeline::instance()->reportResults( qid, results );
}


void
QtScriptResolver::stop()
{
    m_stopped = true;
}
