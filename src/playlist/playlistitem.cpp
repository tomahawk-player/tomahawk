#include "playlistitem.h"

#include "utils/tomahawkutils.h"

#include <QDebug>

using namespace Tomahawk;


PlaylistItem::PlaylistItem( const Tomahawk::query_ptr& query, QObject* parent )
    : QObject( parent )
{
    setupItem( query );
}


PlaylistItem::PlaylistItem( const Tomahawk::plentry_ptr& entry, QObject* parent )
    : QObject( parent )
    , m_entry( entry )
{
    setupItem( entry->query() );
}


void
PlaylistItem::setupItem( const Tomahawk::query_ptr& query )
{
    m_beingRemoved = false;
    m_query = query;

    QVariantMap map = query->toVariant().toMap();
    qlonglong ptr = qlonglong( this );

    QStandardItem* item = new QStandardItem( map.value( "artist" ).toString() );
    item->setData( ptr, Qt::UserRole );
    item->setSizeHint( QSize( 0, 18 ) );
    m_columns << item;

    item = new QStandardItem( map.value( "track" ).toString() );
    m_columns << item;

    item = new QStandardItem( map.value( "album" ).toString() );
    m_columns << item;

    item = new QStandardItem( TomahawkUtils::timeToString( map.value( "duration" ).toInt() ) );
    m_columns << item;

    item = new QStandardItem( map.value( "bitrate" ).toString() );
    m_columns << item;

    item = new QStandardItem( "" ); // sources
    m_columns << item;

    foreach( QStandardItem* item, m_columns )
        item->setEditable( false );

    if ( query->numResults() )
        onResultsAdded( query->results() );

    connect( query.data(), SIGNAL( resultsAdded( const QList<Tomahawk::result_ptr>& ) ),
                             SLOT( onResultsAdded( const QList<Tomahawk::result_ptr>& ) ), Qt::DirectConnection );
}


void
PlaylistItem::onResultsAdded( const QList<Tomahawk::result_ptr>& results )
{
    //qDebug() << "Found results for playlist item:" << this;
    const Tomahawk::result_ptr& result = m_query->results().at( 0 );

    // Since we have a result now, we can enable the PlaylistItem and update the actual metadata
    m_columns.at( 0 )->setText( result->artist() );
    m_columns.at( 1 )->setText( result->track() );
    m_columns.at( 2 )->setText( result->album() );
    m_columns.at( 3 )->setText( TomahawkUtils::timeToString( result->duration() ) );
    m_columns.at( 4 )->setText( QString::number( result->bitrate() ) );

    if ( m_query->results().count() > 1 )
    {
        // count unique sources
        QList<unsigned int> uniqsrcs;
        unsigned int c = m_query->results().count();
        for ( unsigned int i = 0; i < c; i++ )
        {
            if ( !uniqsrcs.contains( m_query->results().at( i )->collection()->source()->id() ) )
                uniqsrcs.append( m_query->results().at( i )->collection()->source()->id() );
        }

        m_columns.at( 5 )->setText( QString( "%1%2" )
                                       .arg( result->collection()->source()->friendlyName() )
                                       .arg( uniqsrcs.count() > 1 ? QString( " (%1)" ).arg( uniqsrcs.count() ) : "" ) );
    }
    else
        m_columns.at( 5 )->setText( result->collection()->source()->friendlyName() );

    foreach( QStandardItem* item, m_columns )
    {
        item->setEnabled( false ); // FIXME: not exactly elegant
        item->setEnabled( true );
    }
}


QModelIndex
PlaylistItem::index() const
{
    if ( m_columns.length() )
        return m_columns.at( 0 )->index();

    return QModelIndex();
}


void
PlaylistItem::setBeingRemoved( bool state )
{
    m_beingRemoved = state;
}
