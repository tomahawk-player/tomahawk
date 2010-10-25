#include "plitem.h"

#include "utils/tomahawkutils.h"

#include <QDebug>

using namespace Tomahawk;


PlItem::~PlItem()
{
    qDeleteAll( children );

//    Q_ASSERT( parent->children.at( m_parentPos ) == this );

    if ( parent )
        parent->children.removeAt( m_parentPos );
}


PlItem::PlItem( PlItem* parent, QAbstractItemModel* model )
{
    this->parent = parent;
    this->model = model;
    childCount = 0;
    toberemoved = false;

    if ( parent )
    {
        parent->children.append( this );
        m_parentPos = parent->children.count() - 1;
    }
}


PlItem::PlItem( const QString& caption, PlItem* parent )
{
    this->parent = parent;
    this->caption = caption;
    this->model = parent->model;
    childCount = 0;
    m_isPlaying = false;
    toberemoved = false;

    if ( parent )
    {
        parent->children.append( this );
        m_parentPos = parent->children.count() - 1;
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
        m_parentPos = parent->children.count() - 1;
        this->model = parent->model;

        connect( model, SIGNAL( rowsRemoved( QModelIndex, int, int ) ),
                          SLOT( onModelRowsRemoved( QModelIndex, int, int ) ) );
    }

    m_isPlaying = false;
    toberemoved = false;
    m_query = query;
    if ( query->numResults() )
        onResultsAdded( query->results() );

    connect( query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ),
                             SLOT( onResultsAdded( QList<Tomahawk::result_ptr> ) ), Qt::DirectConnection );

    connect( query.data(), SIGNAL( resultsRemoved( Tomahawk::result_ptr ) ),
                             SLOT( onResultsRemoved( Tomahawk::result_ptr ) ), Qt::DirectConnection );
}


void
PlItem::onResultsAdded( const QList<Tomahawk::result_ptr>& results )
{
//    qDebug() << "Found results for playlist item:" << this;
    emit dataChanged();
}


void
PlItem::onResultsRemoved( const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO;
    emit dataChanged();
}


void
PlItem::onModelRowsRemoved( const QModelIndex& index, int start, int end )
{
    if ( !toberemoved && this->parent->index == index )
    {
        if ( ( start <= m_parentPos ) && ( m_parentPos <= end ) )
            toberemoved = true;
        else
        {
            if ( start < m_parentPos )
            {
                m_parentPos -= ( end - start ) + 1;
            }
        }
    }
}
