/*
    Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "sourcetreeitem.h"

#include "utils/Logger.h"

using namespace Tomahawk;


SourceTreeItem::SourceTreeItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::RowType thisType, int peerSortValue, int index )
    : QObject()
    , m_type( thisType )
    , m_parent( parent )
    , m_model( model )
    , m_peerSortValue( peerSortValue )
{
    connect( this, SIGNAL( beginChildRowsAdded( int, int ) ), m_model, SLOT( onItemRowsAddedBegin( int, int ) ) );
    connect( this, SIGNAL( beginChildRowsRemoved( int, int ) ), m_model, SLOT( onItemRowsRemovedBegin( int, int ) ) );
    connect( this, SIGNAL( childRowsAdded() ), m_model, SLOT( onItemRowsAddedDone() ) );
    connect( this, SIGNAL( childRowsRemoved() ), m_model, SLOT( onItemRowsRemovedDone() ) );
    connect( this, SIGNAL( updated() ), m_model, SLOT( itemUpdated() ) );
    connect( this, SIGNAL( selectRequest( SourceTreeItem* ) ), m_model, SLOT( itemSelectRequest( SourceTreeItem* ) ) );
    connect( this, SIGNAL( expandRequest( SourceTreeItem* ) ), m_model, SLOT( itemExpandRequest( SourceTreeItem* ) ) );
    connect( this, SIGNAL( toggleExpandRequest( SourceTreeItem* ) ), m_model, SLOT( itemToggleExpandRequest( SourceTreeItem* ) ) );

    if ( !m_parent )
        return;

    // caller must call begin/endInsertRows
    if ( index < 0 )
        m_parent->appendChild( this );
    else
        m_parent->insertChild( index, this );
}


SourceTreeItem::~SourceTreeItem()
{
    qDeleteAll( m_children );
}
