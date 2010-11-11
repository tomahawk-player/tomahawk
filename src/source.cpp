#include "tomahawk/source.h"

#include "tomahawk/tomahawkapp.h"
#include "tomahawk/collection.h"

#include "controlconnection.h"
#include "databasecommand_addsource.h"
#include "databasecommand_sourceoffline.h"
#include "database.h"

using namespace Tomahawk;


Source::Source( const QString &username, ControlConnection* cc )
    : QObject()
    , m_isLocal( false )
    , m_online( false )
    , m_username( username )
    , m_id( 0 )
    , m_cc( cc )
{
    // source for local machine doesn't have a controlconnection. this is normal.
    if ( cc )
        connect( cc, SIGNAL( finished() ), SLOT( remove() ), Qt::QueuedConnection );
}


Source::Source( const QString &username )
    : QObject()
    , m_isLocal( true )
    , m_online( false )
    , m_username( username )
    , m_id( 0 )
    , m_cc( 0 )
{
}


Source::~Source()
{
    qDebug() << Q_FUNC_INFO;
    // TODO mark source as offline in database
    DatabaseCommand_SourceOffline * cmd = new DatabaseCommand_SourceOffline( id() );
    APP->database()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


collection_ptr
Source::collection() const
{
    if( m_collections.length() )
        return m_collections.first();

    collection_ptr tmp;
    return tmp;
}


void
Source::doDBSync()
{
    // ensure username is in the database
    DatabaseCommand_addSource * cmd = new DatabaseCommand_addSource( m_username, m_friendlyname );
    connect( cmd, SIGNAL( done( unsigned int, QString ) ),
                    SLOT( dbLoaded( unsigned int, const QString& ) ) );
    APP->database()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


void
Source::setStats( const QVariantMap& m )
{
    m_stats = m;
    emit stats( m_stats );
}


void
Source::remove()
{
    qDebug() << Q_FUNC_INFO;

    m_cc = 0;
    emit offline();
    APP->sourcelist().remove( this );
    m_collections.clear();
}


QString
Source::friendlyName() const
{
    if ( m_friendlyname.isEmpty() )
        return m_username;

    if ( m_friendlyname.contains( "/tomahawk" ) )
        return m_friendlyname.left( m_friendlyname.indexOf( "/tomahawk" ) );

    return m_friendlyname;
}


void
Source::addCollection( collection_ptr c )
{
    Q_ASSERT( m_collections.length() == 0 ); // only 1 source supported atm
    m_collections.append( c );
    emit collectionAdded( c );
}


void
Source::removeCollection( collection_ptr c )
{
    Q_ASSERT( m_collections.length() == 1 && m_collections.first() == c ); // only 1 source supported atm
    m_collections.removeAll( c );
    emit collectionRemoved( c );
}


void
Source::setOffline()
{
    if ( !m_online )
        return;

    m_online = false;
    emit offline();
}


void
Source::setOnline()
{
    if ( m_online )
        return;

    m_online = true;
    emit online();
}


void
Source::dbLoaded( unsigned int id, const QString& fname )
{
    qDebug() << Q_FUNC_INFO << id << fname;
    m_id = id;
    m_friendlyname = fname;
    emit syncedWithDatabase();
}
