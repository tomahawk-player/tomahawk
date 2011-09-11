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

#include "checkdirtree.h"

#include "utils/logger.h"


CheckDirModel::CheckDirModel( QWidget* parent )
    : QDirModel( parent )
{
    setLazyChildCount( false );
}


Qt::ItemFlags
CheckDirModel::flags( const QModelIndex& index ) const
{
    return QDirModel::flags( index ) | Qt::ItemIsUserCheckable;
}


QVariant
CheckDirModel::data( const QModelIndex& index, int role ) const
{
    if ( role == Qt::CheckStateRole )
    {
        return m_checkTable.contains( index ) ? m_checkTable.value( index ) : Qt::Unchecked;
    }
    else
    {
        return QDirModel::data( index, role );
    }
}


bool
CheckDirModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    bool b = QDirModel::setData( index, value, role );

    if ( role == Qt::CheckStateRole )
    {
        m_checkTable.insert( index, (Qt::CheckState)value.toInt() );
        emit dataChanged( index, index );
        emit dataChangedByUser( index );
    }

    return b;
}


void
CheckDirModel::setCheck( const QModelIndex& index, const QVariant& value )
{
    QDirModel::setData( index, value, Qt::CheckStateRole );
    m_checkTable.insert( index, (Qt::CheckState)value.toInt() );
    emit dataChanged( index, index );
}


Qt::CheckState
CheckDirModel::getCheck( const QModelIndex& index )
{
    return (Qt::CheckState)data( index, Qt::CheckStateRole ).toInt();
}


CheckDirTree::CheckDirTree( QWidget* parent )
    : QTreeView( parent )
{
    m_dirModel.setFilter( QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks );
    setModel( &m_dirModel );
    setColumnHidden( 1, true );
    setColumnHidden( 2, true );
    setColumnHidden( 3, true );
    //header()->hide();

    connect( &m_dirModel, SIGNAL( dataChangedByUser( QModelIndex ) ),
                            SLOT( updateNode( QModelIndex ) ) );
    connect( &m_dirModel, SIGNAL( dataChangedByUser( const QModelIndex& ) ),
                          SIGNAL( changed() ) );

    connect( this, SIGNAL( collapsed( QModelIndex ) ),
                     SLOT( onCollapse( QModelIndex ) ) );
    connect( this, SIGNAL( expanded( QModelIndex ) ),
                     SLOT( onExpand( QModelIndex ) ) );
}


void
CheckDirTree::checkPath( const QString& path, Qt::CheckState state )
{
    QModelIndex index = m_dirModel.index( path );
    m_dirModel.setCheck( index, state );
    updateNode( index );
}


void
CheckDirTree::setExclusions( const QStringList& list )
{
    foreach ( const QString& path, list )
    {
        checkPath( path, Qt::Unchecked );
    }
}


QStringList
CheckDirTree::getCheckedPaths()
{
    QStringList checks;
    QModelIndex root = rootIndex();

    getChecksForNode( root, checks );
    return checks;
}


void
CheckDirTree::getChecksForNode( const QModelIndex& index, QStringList& checks )
{
    // Look at first node
    // Is it checked?
    //  - move on to next node
    // Is it unchecked?
    //  - add to list
    //  - move to next node
    // Is it partially checked?
    //  - recurse

    int numChildren = m_dirModel.rowCount( index );
    for ( int i = 0; i < numChildren; ++i )
    {
        QModelIndex kid = m_dirModel.index( i, 0, index );
        Qt::CheckState check = m_dirModel.getCheck( kid );
        if ( check == Qt::Checked )
        {
            checks.append( m_dirModel.filePath( kid ) );
        }
        else if ( check == Qt::Unchecked )
        {
            continue;
        }
        else if ( check == Qt::PartiallyChecked )
        {
            getChecksForNode( kid, checks );
        }
        else
        {
            Q_ASSERT( false );
        }
    }
}


QStringList
CheckDirTree::getExclusions()
{
    QStringList exclusions;
    QModelIndex root = rootIndex();

    getExclusionsForNode( root, exclusions );
    return exclusions;
}


void
CheckDirTree::getExclusionsForNode( const QModelIndex& index, QStringList& exclusions )
{
    // Look at first node
    // Is it checked?
    //  - move on to next node
    // Is it unchecked?
    //  - add to list
    //  - move to next node
    // Is it partially checked?
    //  - recurse

    int numChildren = m_dirModel.rowCount( index );
    for ( int i = 0; i < numChildren; ++i )
    {
        QModelIndex kid = m_dirModel.index( i, 0, index );
        Qt::CheckState check = m_dirModel.getCheck( kid );
        if ( check == Qt::Checked )
        {
            continue;
        }
        else if ( check == Qt::Unchecked )
        {
            exclusions.append( m_dirModel.filePath( kid ) );
        }
        else if ( check == Qt::PartiallyChecked )
        {
            getExclusionsForNode( kid, exclusions );
        }
        else
        {
            Q_ASSERT( false );
        }
    }
}


void
CheckDirTree::onCollapse( const QModelIndex& /*idx*/ )
{

}


void
CheckDirTree::onExpand( const QModelIndex& idx )
{
    // If the node is partially checked, that means we have been below it
    // setting some stuff, so only fill down if we are unchecked.
    if ( m_dirModel.getCheck( idx ) != Qt::PartiallyChecked )
    {
        fillDown( idx );
    }
}


void
CheckDirTree::updateNode( const QModelIndex& idx )
{
    // Start by recursing down to the bottom and then work upwards
    fillDown( idx );
    updateParent( idx );
}


void
CheckDirTree::fillDown( const QModelIndex& parent )
{
    // Recursion stops when we reach a directory which has never been expanded
    // or one that has no children.
    if ( !isExpanded( parent ) || !m_dirModel.hasChildren( parent ) )
    {
        return;
    }

    Qt::CheckState state = m_dirModel.getCheck( parent );
    int numChildren = m_dirModel.rowCount( parent );
    for ( int i = 0; i < numChildren; ++i )
    {
        QModelIndex kid = m_dirModel.index( i, 0, parent );
        m_dirModel.setCheck( kid, state );
        fillDown( kid );
    }
}


void
CheckDirTree::updateParent( const QModelIndex& index )
{
    QModelIndex parent = index.parent();
    if ( !parent.isValid() )
    {
        // We have reached the root
        return;
    }

    // Initialise overall state to state of first child
    QModelIndex kid = m_dirModel.index( 0, 0, parent );
    Qt::CheckState overall = m_dirModel.getCheck( kid );

    int numChildren = m_dirModel.rowCount( parent );
    for ( int i = 1; i <= numChildren; ++i )
    {
        kid = m_dirModel.index( i, 0, parent );
        Qt::CheckState state = m_dirModel.getCheck( kid );
        if ( state != overall )
        {
            // If we ever come across a state different than the first child,
            // we are partially checked
            overall = Qt::PartiallyChecked;
            break;
        }
    }

    m_dirModel.setCheck( parent, overall );
    updateParent( parent );
}
