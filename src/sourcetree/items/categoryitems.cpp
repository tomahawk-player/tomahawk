/*
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "categoryitems.h"

#include "utils/tomahawkutils.h"
#include "tomahawk/tomahawkapp.h"
#include "widgets/newplaylistwidget.h"
#include "viewmanager.h"
#include "viewpage.h"

using namespace Tomahawk;

/// CategoryAddItem

CategoryAddItem::CategoryAddItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::CategoryType type )
    : SourceTreeItem( model, parent, SourcesModel::CategoryAdd )
    , m_categoryType( type )
{

}

CategoryAddItem::~CategoryAddItem()
{

}

QString
CategoryAddItem::text() const
{
    switch( m_categoryType ) {
        case SourcesModel::PlaylistsCategory:
            return tr( "New Playlist" );
        case SourcesModel::StationsCategory:
            return tr( "New Station" );
    }

    return QString();
}

void
CategoryAddItem::activate()
{
    switch( m_categoryType )
    {
        case SourcesModel::PlaylistsCategory:
            // only show if none is shown yet
            if( !ViewManager::instance()->isNewPlaylistPageVisible() ) {
                ViewPage* p = ViewManager::instance()->show( new NewPlaylistWidget() );
                model()->linkSourceItemToPage( this, p );
            }
            break;
        case SourcesModel::StationsCategory:
            APP->mainWindow()->createStation();
            break;
    }
}

Qt::ItemFlags
CategoryAddItem::flags() const
{
    switch( m_categoryType )
    {
        case SourcesModel::PlaylistsCategory:
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        case SourcesModel::StationsCategory:
        default:
            return Qt::ItemIsEnabled;
            break;
    }
}

QIcon
CategoryAddItem::icon() const
{
    return QIcon( RESPATH "images/add.png" );
}

/// CategoryItem

CategoryItem::CategoryItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::CategoryType category, bool showAddItem )
    : SourceTreeItem( model, parent, SourcesModel::Category )
    , m_category( category )
    , m_addItem( 0 )
    , m_showAdd( showAddItem )
{
    // in the constructor we're still being added to the parent, so we don't exist to have rows addded yet. so this is safe.
    //     beginRowsAdded( 0, 0 );
    if( m_showAdd ) {
        m_addItem = new CategoryAddItem( model, this, m_category );
    }
    //     endRowsAdded();
}

void
CategoryItem::insertItem( SourceTreeItem* item )
{
    insertItems( QList< SourceTreeItem* >() << item );
}

void
CategoryItem::insertItems( QList< SourceTreeItem* > items )
{
    // add the items to the category, and connect to the signals
    int curCount = children().size();
    if( m_showAdd ) // if there's an add item, add it before that
        curCount--;
    beginRowsAdded( curCount, curCount + items.size() - 1 );
    foreach( SourceTreeItem* item, items ) {
        int index = m_showAdd ? children().count() - 1 : children().count();
        insertChild( children().count() - 1, item );
    }
    endRowsAdded();
}


void
CategoryItem::activate()
{
    if( m_category == SourcesModel::StationsCategory ) {
        // TODO activate stations page
    }
}