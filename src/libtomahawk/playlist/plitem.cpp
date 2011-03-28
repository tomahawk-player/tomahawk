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

#include "plitem.h"

#include "utils/tomahawkutils.h"
#include "playlist.h"
#include "query.h"

#include <QDebug>

using namespace Tomahawk;


PlItem::~PlItem()
{
    // Don't use qDeleteAll here! The children will remove themselves
    // from the list when they get deleted and the qDeleteAll iterator
    // will fail badly!
    if ( parent && index.isValid() )
    {
        parent->children.remove( index.row() );
    }

    for ( int i = children.count() - 1; i >= 0; i-- )
        delete children.at( i );
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
    }
}


PlItem::PlItem( const Tomahawk::query_ptr& query, PlItem* parent, int row )
    : QObject( parent )
{
    setupItem( query, parent, row );
}


PlItem::PlItem( const Tomahawk::plentry_ptr& entry, PlItem* parent, int row )
    : QObject( parent )
    , m_entry( entry )
{
    setupItem( entry->query(), parent, row );
}

const Tomahawk::plentry_ptr& 
PlItem::entry() const
{
    return m_entry;
}

const Tomahawk::query_ptr& 
PlItem::query() const
{
    if ( !m_entry.isNull() ) return m_entry->query(); else return m_query;
}


void
PlItem::setupItem( const Tomahawk::query_ptr& query, PlItem* parent, int row )
{
    this->parent = parent;
    if ( parent )
    {
        if ( row < 0 )
        {
            parent->children.append( this );
            row = parent->children.count() - 1;
        }
        else
        {
            parent->children.insert( row, this );
        }

        this->model = parent->model;
    }

    m_isPlaying = false;
    toberemoved = false;
    m_query = query;
    if ( query->numResults() )
    {
        emit dataChanged();
    }
    else
    {
        connect( query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ),
                               SIGNAL( dataChanged() ) );

        connect( query.data(), SIGNAL( resultsRemoved( Tomahawk::result_ptr ) ),
                               SIGNAL( dataChanged() ) );
        
        connect( query.data(), SIGNAL( resultsChanged() ),
                               SIGNAL( dataChanged() ) );
    }
}
