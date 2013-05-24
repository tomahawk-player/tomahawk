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

#include "SourceTreeItem.h"

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


SourcesModel::RowType
SourceTreeItem::type() const
{
    return m_type;
}


SourceTreeItem*
SourceTreeItem::parent() const
{
    return m_parent;
}


SourcesModel*
SourceTreeItem::model() const
{
    return m_model;
}


QList< SourceTreeItem* >
SourceTreeItem::children() const
{
    return m_children;
}


void
SourceTreeItem::appendChild(SourceTreeItem* item)
{
    m_children.append( item );
}


void
SourceTreeItem::insertChild(int index, SourceTreeItem* item)
{
    m_children.insert( index, item );
}


void
SourceTreeItem::removeChild(SourceTreeItem* item)
{
    m_children.removeAll( item );
}


QString
SourceTreeItem::text() const
{
    return QString();
}


QString
SourceTreeItem::tooltip() const
{
    return QString();
}


Qt::ItemFlags
SourceTreeItem::flags() const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


QIcon
SourceTreeItem::icon() const
{
    return QIcon();
}


bool
SourceTreeItem::willAcceptDrag(const QMimeData*) const
{
    return false;
}


bool
SourceTreeItem::dropMimeData(const QMimeData*, Qt::DropAction)
{
    return false;
}


bool
SourceTreeItem::setData(const QVariant&, bool)
{
    return false;
}


int
SourceTreeItem::peerSortValue() const
{
    return m_peerSortValue;
}


int
SourceTreeItem::IDValue() const
{
    return 0;
}


SourceTreeItem::DropTypes
SourceTreeItem::supportedDropTypes(const QMimeData* mimeData) const
{
    Q_UNUSED( mimeData );
    return DropTypesNone;
}


void
SourceTreeItem::setDropType(SourceTreeItem::DropType type)
{
    m_dropType = type;
}


SourceTreeItem::DropType
SourceTreeItem::dropType() const
{
    return m_dropType;
}


bool
SourceTreeItem::isBeingPlayed() const
{
    return false;
}


QList< QAction* >
SourceTreeItem::customActions() const
{
    return QList< QAction* >();
}


void
SourceTreeItem::beginRowsAdded(int from, int to)
{
    emit beginChildRowsAdded( from, to );
}


void
SourceTreeItem::endRowsAdded()
{
    emit childRowsAdded();
}


void
SourceTreeItem::beginRowsRemoved(int from, int to)
{
    emit beginChildRowsRemoved( from, to );
}


void
SourceTreeItem::endRowsRemoved()
{
    emit childRowsRemoved();
}


void
SourceTreeItem::setRowType(SourcesModel::RowType t)
{
    m_type = t;
}


void
SourceTreeItem::setParentItem(SourceTreeItem* item)
{
    m_parent = item;
}
