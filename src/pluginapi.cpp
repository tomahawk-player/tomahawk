#include "tomahawk/pluginapi.h"
#include "tomahawk/tomahawkapp.h"

using namespace Tomahawk;

PluginAPI::PluginAPI( Pipeline* p )
    : m_pipeline( p )
{
}


/*void
PluginAPI::reportResults( const QString& qid, const QList<QVariantMap>& vresults )
{
    QList< result_ptr > results;
    foreach( QVariantMap m, vresults )
    {
        result_ptr rp( new Result( m ) );
        results.append( rp );
    }
    m_pipeline->reportResults( QID( qid ), results );
}*/


void
PluginAPI::addSource( source_ptr s )
{
    TomahawkApp::instance()->sourcelist().add( s );
}


void
PluginAPI::removeSource( source_ptr s )
{
    TomahawkApp::instance()->sourcelist().remove( s );
}


void
PluginAPI::addResolver( Resolver* r )
{
    TomahawkApp::instance()->pipeline()->addResolver( r );
}
