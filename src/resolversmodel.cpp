/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "tomahawksettings.h"
#include "tomahawkapp.h"
#include "resolver.h"
#include "pipeline.h"
#include "config.h"
#include "AtticaManager.h"
#include "utils/logger.h"

ResolversModel::ResolversModel( QObject* parent )
    : QAbstractListModel( parent )
{
     addInstalledResolvers();
}


ResolversModel::~ResolversModel()
{

}

QVariant
ResolversModel::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() || !hasIndex( index.row(), index.column(), QModelIndex() ) )
        return QVariant();

    Tomahawk::ExternalResolver* res = Tomahawk::Pipeline::instance()->scriptResolvers().at( index.row() );
    switch( role )
    {
    case Qt::DisplayRole:
    case ResolversModel::ResolverName:
        return res->name();
    case ResolversModel::ResolverPath:
        return res->filePath();
    case ResolversModel::HasConfig:
        return res->configUI() != 0;
    case ResolversModel::ErrorState:
        return res->error();
    case Qt::CheckStateRole:
        return res->running() ? Qt::Checked : Qt::Unchecked;
    case Qt::ToolTipRole:
        return res->filePath();
    default:
        return QVariant();
    }
}

bool
ResolversModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if ( !hasIndex( index.row(), index.column(), QModelIndex() ) )
        return false;

    Tomahawk::ExternalResolver* r = Tomahawk::Pipeline::instance()->scriptResolvers().at( index.row() );
    if ( r && r->error() == Tomahawk::ExternalResolver::FileNotFound )  // give it a shot to see if the user manually fixed paths
    {
        r->reload();

        if( r->error() == Tomahawk::ExternalResolver::FileNotFound ) // Nope, no luck. Doesn't exist on disk, don't let user mess with it
            return false;
    }
    else if ( !r && !QFile::exists( r->filePath() ) )
    {
        return false;
    }

    if ( role == Qt::CheckStateRole )
    {
        Qt::CheckState state = static_cast< Qt::CheckState >( value.toInt() );

        if ( state == Qt::Checked && !r->running() ) {
            r->start();
        }
        else if ( state == Qt::Unchecked )
        {
            r->stop();
        }

        emit dataChanged( index, index );
        return true;
    }
    return false;
}


int
ResolversModel::rowCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return Tomahawk::Pipeline::instance()->scriptResolvers().count();
}

int
ResolversModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED( parent );
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
    const int count = rowCount( QModelIndex() );
    beginInsertRows( QModelIndex(), count, count );
    Tomahawk::ExternalResolver* res = Tomahawk::Pipeline::instance()->addScriptResolver( resolver, enable );
    connect( res, SIGNAL( changed() ), this, SLOT( resolverChanged() ) );
    endInsertRows();

    if ( res->configUI() )
        emit openConfig( res->filePath() );
    else
        m_waitingForLoad << resolver;
}

void
ResolversModel::atticaResolverInstalled( const QString& resolverId )
{
#ifdef LIBATTICA_FOUND
    Tomahawk::ExternalResolver* r = Tomahawk::Pipeline::instance()->resolverForPath( AtticaManager::instance()->pathFromId( resolverId ) );
    if ( !r )
        return;
    const int idx = Tomahawk::Pipeline::instance()->scriptResolvers().indexOf( r );
    if ( idx >= 0 )
    {
        beginInsertRows( QModelIndex(), idx, idx );
        endInsertRows();
    }
#endif
}


void
ResolversModel::removeResolver( const QString& resolver )
{
    const int idx = Tomahawk::Pipeline::instance()->scriptResolvers().indexOf( Tomahawk::Pipeline::instance()->resolverForPath( resolver ) );
    if ( idx < 0 )
        return;

    beginRemoveRows( QModelIndex(), idx, idx );
    Tomahawk::Pipeline::instance()->removeScriptResolver( resolver );
    endRemoveRows();
}

void
ResolversModel::resolverChanged()
{
    Tomahawk::ExternalResolver* res = qobject_cast< Tomahawk::ExternalResolver* >( sender() );
    Q_ASSERT( res );

    if ( Tomahawk::Pipeline::instance()->scriptResolvers().contains( res ) )
    {
        qDebug() << "Got resolverChanged signal, does it have a config UI yet?" << res->configUI();
        const QModelIndex idx = index( Tomahawk::Pipeline::instance()->scriptResolvers().indexOf( res ), 0, QModelIndex() );
        emit dataChanged( idx, idx );

        if ( m_waitingForLoad.contains( res->filePath() ) )
        {
            m_waitingForLoad.remove( res->filePath() );
            emit openConfig( res->filePath() );
        }
    }
}

void
ResolversModel::addInstalledResolvers()
{
    QList< QDir > pluginDirs;

    QDir appDir( qApp->applicationDirPath() );
    QDir libDir( CMAKE_INSTALL_PREFIX "/lib" );
    QDir libexecDir( CMAKE_INSTALL_LIBEXECDIR );

    QDir lib64Dir( appDir );
    lib64Dir.cdUp();
    lib64Dir.cd( "lib64" );

    pluginDirs << appDir << libDir << lib64Dir << libexecDir << QDir( qApp->applicationDirPath() );
    foreach ( const QDir& pluginDir, pluginDirs )
    {
        qDebug() << "Checking directory for resolvers:" << pluginDir;
        foreach ( QString fileName, pluginDir.entryList( QStringList() << "*_tomahawkresolver*", QDir::Files ) ){
            if ( fileName.contains( "_tomahawkresolver" ) ) {
                const QString path = pluginDir.absoluteFilePath( fileName );
                bool found = false;
                foreach ( Tomahawk::ExternalResolver* res, Tomahawk::Pipeline::instance()->scriptResolvers() )
                {
                    if ( res->filePath() == path )
                        found = true;
                }
                if ( !found ) {
                    Tomahawk::Pipeline::instance()->addScriptResolver( path, false );
                }
            }
        }
    }
}

void
ResolversModel::saveScriptResolvers()
{
    QStringList enabled, all;
    foreach ( Tomahawk::ExternalResolver* res, Tomahawk::Pipeline::instance()->scriptResolvers() )
    {
        all << res->filePath();
        if ( res->running() )
            enabled << res->filePath();
    }
    TomahawkSettings::instance()->setAllScriptResolvers( all );
    TomahawkSettings::instance()->setEnabledScriptResolvers( enabled );
}
