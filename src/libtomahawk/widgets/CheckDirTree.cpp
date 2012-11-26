/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011,      Leo Franchi <lfranchi@kde.org>
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

#include "CheckDirTree.h"

#include "utils/Logger.h"
#include "TomahawkSettings.h"
#include "Source.h"

#include <QCoreApplication>
#include <QProcess>

static QString s_macVolumePath = "/Volumes";

CheckDirModel::CheckDirModel( QWidget* parent )
    : QFileSystemModel( parent )
    , m_shownVolumes( false )
{
#ifdef Q_OS_MAC
    m_setFilePath = QString( "%1/SetFile" )        .arg( QCoreApplication::applicationDirPath() );
    m_getFileInfoPath = QString( "%1/GetFileInfo" ).arg( QCoreApplication::applicationDirPath() );

    QProcess* checkVolumeVisible = new QProcess( this );
    connect( checkVolumeVisible, SIGNAL( readyReadStandardOutput() ), this, SLOT( getFileInfoResult() ) );
    qDebug() << "Running GetFileInfo:" << m_getFileInfoPath << "-aV" << s_macVolumePath;
    checkVolumeVisible->start( m_getFileInfoPath, QStringList() <<  "-aV" << s_macVolumePath );
#endif
}

CheckDirModel::~CheckDirModel()
{
    cleanup();
}


void
CheckDirModel::cleanup()
{
#ifdef Q_OS_MAC
    // reset to previous state
    if ( m_shownVolumes )
        QProcess::startDetached( QString( "%1 -a V %2" ).arg( m_setFilePath).arg( s_macVolumePath ) );
#endif
}

void
CheckDirModel::getFileInfoResult()
{
#ifdef Q_OS_MAC
    QProcess* p = qobject_cast< QProcess* >( sender() );
    Q_ASSERT( p );

    QByteArray res = p->readAll().trimmed();
    qDebug() << "Got output from GetFileInfo:" << res;
    // 1 means /Volumes is hidden, so we show it while the dialog is visible
    if ( res == "1" )
    {
        // Remove the hidden flag for the /Volumnes folder so all mount points are visible in the default (Q)FileSystemModel
        QProcess* showProcess = new QProcess( this );
        qDebug() << "Running SetFile:" << QString( "%1 -a v %2" ).arg( m_setFilePath ).arg( s_macVolumePath );
        showProcess->start( QString( "%1 -a v %2" ).arg( m_setFilePath ).arg( s_macVolumePath ) );
        connect( showProcess, SIGNAL( readyReadStandardError() ), this, SLOT( processErrorOutput() ) );
        m_shownVolumes = true;

        QTimer::singleShot( 500, this, SLOT( volumeShowFinished() ) );
    }

    p->terminate();
    p->deleteLater();
#endif
}

void
CheckDirModel::volumeShowFinished()
{
    reset();

#ifdef Q_OS_MAC
    // Make sure /Volumes is there, if not wait and try again
    const QModelIndex parent = index("/");
    const int count = rowCount(parent);
    bool found = false;
    for ( int i = 0; i < count; i++ )
    {
        if ( data( index( i, 0, parent ) ).toString() == "Volumes" )
        {
            found = true;
            break;
        }
    }
    if ( !found )
        QTimer::singleShot( 500, this, SLOT( volumeShowFinished() ) );
#endif
}


void
CheckDirModel::processErrorOutput()
{
    QProcess* p = qobject_cast< QProcess* >( sender() );
    Q_ASSERT( p );
    qDebug() << "Got ERROR OUTPUT from subprocess in CheckDirModel:" << p->readAll();
}


Qt::ItemFlags
CheckDirModel::flags( const QModelIndex& index ) const
{
    return QFileSystemModel::flags( index ) | Qt::ItemIsUserCheckable;
}


QVariant
CheckDirModel::data( const QModelIndex& index, int role ) const
{
#ifdef Q_WS_MAC
    // return the 'My Computer' icon for the /Volumes folder
    if ( index.column() == 0 && filePath( index ) == s_macVolumePath )
    {
        switch ( role )
        {
            case Qt::DecorationRole:
                return myComputer( role );
            default:
                break;
        }
    }
#endif

    if ( role == Qt::CheckStateRole )
    {
        return m_checkTable.contains( index ) ? m_checkTable.value( index ) : Qt::Unchecked;
    }
    else
    {
        return QFileSystemModel::data( index, role );
    }
}


bool
CheckDirModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    bool b = QFileSystemModel::setData( index, value, role );

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
    QFileSystemModel::setData( index, value, Qt::CheckStateRole );
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
    m_dirModel.setFilter( QDir::Dirs | QDir::NoDotAndDotDot );
    m_dirModel.setRootPath( "/" );

    m_dirModel.setNameFilters( QStringList() << "[^\\.]*" );

    setModel( &m_dirModel );
    setColumnHidden( 1, true );
    setColumnHidden( 2, true );
    setColumnHidden( 3, true );
    //header()->hide();

    connect( &m_dirModel, SIGNAL( dataChangedByUser( QModelIndex ) ),
                            SLOT( updateNode( QModelIndex ) ) );
    connect( &m_dirModel, SIGNAL( dataChangedByUser( const QModelIndex& ) ),
                          SIGNAL( changed() ) );
    connect( &m_dirModel, SIGNAL( modelReset() ),
                            SLOT( modelReset() ) );

    connect( this, SIGNAL( collapsed( QModelIndex ) ),
                     SLOT( onCollapse( QModelIndex ) ) );
    connect( this, SIGNAL( expanded( QModelIndex ) ),
                     SLOT( onExpand( QModelIndex ) ) );
}


void
CheckDirTree::cleanup()
{
    m_dirModel.cleanup();
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
CheckDirTree::modelReset()
{
    foreach ( const QString& dir, TomahawkSettings::instance()->scannerPaths() )
    {
        checkPath( dir, Qt::Checked );
    }
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
