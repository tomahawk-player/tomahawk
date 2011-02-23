#include "sourcesproxymodel.h"

#include <QDebug>
#include <QTreeView>

#include "sourcesmodel.h"
#include "sourcetreeitem.h"


SourcesProxyModel::SourcesProxyModel( SourcesModel* model, QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( model )
    , m_filtered( false )
{
    setDynamicSortFilter( true );

    setSourceModel( model );
}


void
SourcesProxyModel::showOfflineSources()
{
    m_filtered = false;
    invalidateFilter();

//    Q_ASSERT( qobject_cast<QTreeView*>( parent() ) );
//    qobject_cast<QTreeView*>( parent() )->expandAll();
}


void
SourcesProxyModel::hideOfflineSources()
{
    m_filtered = true;
    invalidateFilter();

//    Q_ASSERT( qobject_cast<QTreeView*>( parent() ) );
//    qobject_cast<QTreeView*>( parent() )->expandAll();
}


bool
SourcesProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( !m_filtered )
        return true;
    
    SourceTreeItem* sti = m_model->indexToTreeItem( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( sti )
    {
        if ( sti->source().isNull() || sti->source()->isOnline() )
            return true;
    }

    return false;
}
