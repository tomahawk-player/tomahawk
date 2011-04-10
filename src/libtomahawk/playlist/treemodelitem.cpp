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

#include "treemodelitem.h"

#include "utils/tomahawkutils.h"

#include <QDebug>

using namespace Tomahawk;


TreeModelItem::~TreeModelItem()
{
    // Don't use qDeleteAll here! The children will remove themselves
    // from the list when they get deleted and the qDeleteAll iterator
    // will fail badly!
    for ( int i = children.count() - 1; i >= 0; i-- )
        delete children.at( i );

    if ( parent && index.isValid() )
    {
        parent->children.removeAt( index.row() );
    }
}


TreeModelItem::TreeModelItem( TreeModelItem* parent, QAbstractItemModel* model )
{
    this->parent = parent;
    this->model = model;
    childCount = 0;
    toberemoved = false;
    fetchingMore = false;

    if ( parent )
    {
        parent->children.append( this );
    }
}


TreeModelItem::TreeModelItem( const Tomahawk::album_ptr& album, TreeModelItem* parent, int row )
    : QObject( parent )
    , m_album( album )
{
    this->parent = parent;
    fetchingMore = false;

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

    toberemoved = false;
}


TreeModelItem::TreeModelItem( const Tomahawk::artist_ptr& artist, TreeModelItem* parent, int row )
    : QObject( parent )
    , m_artist( artist )
{
    this->parent = parent;
    fetchingMore = false;

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

    toberemoved = false;
}


TreeModelItem::TreeModelItem( const Tomahawk::result_ptr& result, TreeModelItem* parent, int row )
    : QObject( parent )
    , m_result( result )
{
    this->parent = parent;
    fetchingMore = false;

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

    toberemoved = false;
}
