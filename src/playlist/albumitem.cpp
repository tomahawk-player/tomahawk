#include "albumitem.h"

#include "utils/tomahawkutils.h"

#include <QDebug>

using namespace Tomahawk;


AlbumItem::~AlbumItem()
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


AlbumItem::AlbumItem( AlbumItem* parent, QAbstractItemModel* model )
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


AlbumItem::AlbumItem( const Tomahawk::album_ptr& album, AlbumItem* parent, int row )
    : QObject( parent )
    , m_album( album )
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

    toberemoved = false;
}
