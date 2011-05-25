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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "resolversmodel.h"

#include <QFileInfo>

#include <tomahawksettings.h>
#include <tomahawkapp.h>


ResolversModel::ResolversModel( const QStringList& allResolvers, const QStringList& enabledResolvers, QObject* parent )
    : QAbstractListModel( parent )
    , m_allResolvers( allResolvers )
    , m_enabledResolvers( enabledResolvers )
{
    // do some sanity checking just in case
    bool changed = false;
    foreach( const QString& l, m_enabledResolvers ) {
        if( !m_allResolvers.contains( l ) ) {
            m_enabledResolvers.removeAll( l );
            changed = true;
        }
     }
     if( changed )
        TomahawkSettings::instance()->setEnabledScriptResolvers( m_enabledResolvers );

     addInstalledResolvers();
}


ResolversModel::~ResolversModel()
{

}

QVariant
ResolversModel::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    switch( role )
    {
    case Qt::DisplayRole:
    case ResolversModel::ResolverName:
    {
        QFileInfo info( m_allResolvers.at( index.row() ) );
        return info.baseName();
    }
    case ResolversModel::ResolverPath:
        return m_allResolvers.at( index.row() );
    case ResolversModel::HasConfig:
        if( Tomahawk::ExternalResolver* r = TomahawkApp::instance()->resolverForPath( m_allResolvers.at( index.row() ) ) ) // if we have one, it means we are loaded too!
            return r->configUI() != 0;
        return false;
    case Qt::CheckStateRole:
        return m_enabledResolvers.contains( m_allResolvers.at( index.row() ) ) ? Qt::Checked : Qt::Unchecked;
    default:
        return QVariant();
    }
}

bool
ResolversModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( role == Qt::CheckStateRole ) {
        Qt::CheckState state = static_cast< Qt::CheckState >( value.toInt() );
        QString resolver = m_allResolvers.at( index.row() );

        if( state == Qt::Checked && !m_enabledResolvers.contains( resolver ) ) {
            m_enabledResolvers.append( resolver );

            TomahawkApp::instance()->enableScriptResolver( resolver );
        } else if( state == Qt::Unchecked ) {
            m_enabledResolvers.removeAll( resolver );

            TomahawkApp::instance()->disableScriptResolver( resolver );
        }
        dataChanged( index, index );

        return true;
    }
    return false;
}


int
ResolversModel::rowCount( const QModelIndex& parent ) const
{
    return m_allResolvers.size();
}

int
ResolversModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

Qt::ItemFlags
ResolversModel::flags( const QModelIndex& index ) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
}


void
ResolversModel::addResolver( const QString& resolver, bool enable )
{
    beginInsertRows( QModelIndex(), m_allResolvers.count(), m_allResolvers.count() );
    m_allResolvers << resolver;
    if( enable )
        m_enabledResolvers << resolver;
    if( Tomahawk::ExternalResolver* res = TomahawkApp::instance()->resolverForPath( resolver ) ) {
        qDebug() << "Added resolver with config and stuff:" << res->configUI();
        connect( res, SIGNAL( changed() ), this, SLOT( resolverChanged() ) );
    } else
        qDebug() << "No resolver object for path yet:" << resolver;

    endInsertRows();
}

void
ResolversModel::removeResolver( const QString& resolver )
{
    for( int i = 0; i < m_allResolvers.count(); i++ ) {
        if( m_allResolvers.at( i ) == resolver ) {
            beginRemoveRows( QModelIndex(), i, i );
            m_allResolvers.takeAt( i );
            endRemoveRows();
        }
    }
    m_enabledResolvers.removeAll( resolver );
}

QStringList
ResolversModel::allResolvers() const
{
    return m_allResolvers;
}

QStringList
ResolversModel::enabledResolvers() const
{
    return m_enabledResolvers;
}

void
ResolversModel::resolverChanged()
{
    Q_ASSERT( qobject_cast< Tomahawk::ExternalResolver* >( sender() ) );
    Tomahawk::ExternalResolver* res = qobject_cast< Tomahawk::ExternalResolver* >( sender() );
    qDebug() << "Got resolverChanged signal, does it have a config UI yet?" << res->configUI();
    if( m_enabledResolvers.contains( res->filePath() ) ) {
        QModelIndex idx = index( m_allResolvers.indexOf( res->filePath() ), 0, QModelIndex() );
        emit dataChanged( idx, idx );
    }
}

void
ResolversModel::addInstalledResolvers()
{
    QList< QDir > pluginDirs;

    QDir appDir( qApp->applicationDirPath() );
    QDir libDir( CMAKE_INSTALL_PREFIX "/lib" );

    QDir lib64Dir( appDir );
    lib64Dir.cdUp();
    lib64Dir.cd( "lib64" );

    pluginDirs << appDir << libDir << lib64Dir << QDir( qApp->applicationDirPath() );
    foreach ( const QDir& pluginDir, pluginDirs )
    {
        qDebug() << "Checking directory for resolvers:" << pluginDir;
        foreach ( QString fileName, pluginDir.entryList( QStringList() << "*_tomahawkresolver*", QDir::Files ) ){
            if ( fileName.contains( "_tomahawkresolver" ) ) {
                const QString path = pluginDir.absoluteFilePath( fileName );
                if( !m_allResolvers.contains( path ) ) {
                    m_allResolvers.append( path );
                }
            }
        }
    }
}
