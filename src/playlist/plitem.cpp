#include "plitem.h"

#include "utils/tomahawkutils.h"

#include <QDebug>

using namespace Tomahawk;


PlItem::~PlItem()
{
    parent->children.removeOne( this );
}


PlItem::PlItem( PlItem* parent )
{
    this->parent = parent;
    childCount = 0;

    if ( parent )
    {
        parent->children.append( this );
    }
}


PlItem::PlItem( const QString& caption, PlItem* parent )
{
    this->parent = parent;
    this->caption = caption;
    childCount = 0;
    m_isPlaying = false;

    if ( parent )
    {
        parent->children.append( this );
    }
}


PlItem::PlItem( const Tomahawk::query_ptr& query, PlItem* parent )
    : QObject( parent )
{
    setupItem( query, parent );
}


PlItem::PlItem( const Tomahawk::plentry_ptr& entry, PlItem* parent )
    : QObject( parent )
    , m_entry( entry )
{
    setupItem( entry->query(), parent );
}


void
PlItem::setupItem( const Tomahawk::query_ptr& query, PlItem* parent )
{
    this->parent = parent;
    if ( parent )
    {
        parent->children.append( this );
    }

    m_isPlaying = false;
    m_query = query;
    if ( query->numResults() )
        onResultsAdded( query->results() );

    connect( query.data(), SIGNAL( resultsAdded( const QList<Tomahawk::result_ptr>& ) ),
                             SLOT( onResultsAdded( const QList<Tomahawk::result_ptr>& ) ), Qt::DirectConnection );
}


void
PlItem::onResultsAdded( const QList<Tomahawk::result_ptr>& results )
{
//    qDebug() << "Found results for playlist item:" << this;
    emit dataChanged();
}

