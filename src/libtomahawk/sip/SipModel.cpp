/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "SipModel.h"

#include "tomahawksettings.h"
#include "tomahawk/tomahawkapp.h"
#include "sip/SipHandler.h"

SipModel::SipModel( QObject* parent )
    : QAbstractItemModel( parent )
{
    connect( SipHandler::instance(), SIGNAL( stateChanged( SipPlugin*, SipPlugin::ConnectionState ) ), this, SLOT( pluginStateChanged( SipPlugin* ) ) );
}


SipModel::~SipModel()
{

}

QVariant
SipModel::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    if( index.row() == SipHandler::instance()->allPlugins().count() ) { // last row, this is the factory
        if( role == Qt::DisplayRole )
            return tr( "Add New Account" );
        else if( role == FactoryRole )
            return true;
        else
            return QVariant();
    }

    QList< SipPlugin* > plugins = SipHandler::instance()->allPlugins();
    Q_ASSERT( index.row() <= plugins.size() );
    SipPlugin* p = plugins[ index.row() ];
    switch( role )
    {
        case Qt::DisplayRole:
        case SipModel::PluginName:
            return p->accountName();
        case SipModel::ConnectionStateRole:
            return p->connectionState();
        case SipModel::HasConfig:
            return ( p->configWidget() == 0 );
        case SipModel::FactoryRole:
            return false;
        case Qt::DecorationRole:
            return p->icon();
        case Qt::CheckStateRole:
            return SipHandler::instance()->enabledPlugins().contains( p ) ? Qt::Checked : Qt::Unchecked;
        default:
            return QVariant();
    }
}

bool
SipModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    Q_ASSERT( index.isValid() && index.row() <= SipHandler::instance()->allPlugins().count() );

    if( role == Qt::CheckStateRole ) {
        Qt::CheckState state = static_cast< Qt::CheckState >( value.toInt() );
        QList< SipPlugin* > plugins = SipHandler::instance()->allPlugins();
        SipPlugin* p = plugins[ index.row() ];

        if( state == Qt::Checked && !SipHandler::instance()->enabledPlugins().contains( p ) ) {
            SipHandler::instance()->enablePlugin( p );
        } else if( state == Qt::Unchecked ) {
            SipHandler::instance()->disablePlugin( p );
        }
        dataChanged( index, index );

        return true;
    }
    return false;
}

QModelIndex
SipModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( !parent.isValid() )
        return hasIndex( row, column, parent ) ? createIndex( row, column, 0 ) : QModelIndex();

    // it's a child of the Add Account, e.g. a factory
    if( hasIndex( row, column, parent ) ) {
        createIndex( row, column, 1 /* magic */ );
    }

    return QModelIndex();
}

QModelIndex
SipModel::parent( const QModelIndex& child ) const
{
    if( !child.isValid() )
        return QModelIndex();

    if( child.internalId() == 1 ) {
        return createIndex( SipHandler::instance()->allPlugins().size() - 1, 0, 0 );
    }

    return QModelIndex();
}

int
SipModel::rowCount( const QModelIndex& parent ) const
{
    if( !parent.isValid() ) { // top level item
        if( parent.row() == SipHandler::instance()->allPlugins().count() ) { // last row, this is the factory
            return SipHandler::instance()->pluginFactories().count();
        } else {
            return SipHandler::instance()->allPlugins().size() + 1;
        }
    }
    return 0;
}

int
SipModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

Qt::ItemFlags
SipModel::flags( const QModelIndex& index ) const
{
    return QAbstractItemModel::flags( index ) | Qt::ItemIsUserCheckable;
}

void
SipModel::pluginAdded( SipPlugin* p )
{
    // we assume sip plugins are added at the end of the list.
    Q_ASSERT( SipHandler::instance()->allPlugins().last() == p );
    int size = SipHandler::instance()->allPlugins().count() - 1;
    beginInsertRows( QModelIndex(), size, size );
}

void
SipModel::pluginRemoved( SipPlugin* p )
{
    int idx = SipHandler::instance()->allPlugins().indexOf( p );
    beginRemoveRows( QModelIndex(), idx, idx );
    endRemoveRows();
}

void
SipModel::pluginStateChanged( SipPlugin* p )
{
    int at = SipHandler::instance()->allPlugins().indexOf( p );
    QModelIndex idx = index( at, 0, QModelIndex() );
    emit dataChanged( idx, idx );
}

