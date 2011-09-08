/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "active/tomahawktouchwindow.h"

#include "utils/logger.h"
#include "audio/audioengine.h"
#include "globalactionmanager.h"
#include "sourcesmodel.h"
#include "items/sourcetreeitem.h"
#include "items/collectionitem.h"
#include "viewmanager.h"


#include "libtomahawk/playlist/treeproxymodel.h"

#include <QFileSystemWatcher>
#include <QtDeclarative>

#define QMLGUI "/home/domme/dev/sources/tomahawk-qml"

TomahawkTouchWindow::TomahawkTouchWindow()
    : m_view(0)
    , m_currentPlaylistTreeModel(0)
{
    QFileSystemWatcher* watcher = new QFileSystemWatcher;
    watcher->addPath( QMLGUI );

    connect( watcher, SIGNAL( directoryChanged( QString ) ), SLOT( loadQml() ));
    loadQml();

    setCentralWidget( m_view );
    setWindowTitle("Touch-ma-hawk");
}


TomahawkTouchWindow::~TomahawkTouchWindow()
{

}


void TomahawkTouchWindow::play(const QModelIndex& index )
{
    TreeModelItem* item = m_currentPlaylistTreeModel->sourceModel()->itemFromIndex( m_currentPlaylistTreeModel->mapToSource( index ) );
    if ( item )
    {
        m_currentPlaylistTreeModel->sourceModel()->setCurrentItem( item->index );
        AudioEngine::instance()->playItem( m_currentPlaylistTreeModel, item->result() );
    }
}


void TomahawkTouchWindow::activateItem(const QModelIndex& index)
{
    tLog() << Q_FUNC_INFO << index;
    SourceTreeItem* item = qobject_cast< SourceTreeItem* >( s_sourcesModel->data( index, SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
    item->activate();

    CollectionItem* collectionItem = qobject_cast< CollectionItem* >( item );
    if( collectionItem )
    {
        tLog() << "Activate collectionItem!";
        Tomahawk::collection_ptr collection = collectionItem->source()->collection();

        delete m_currentPlaylistTreeModel;
        m_currentPlaylistTreeModel = new TreeProxyModel();
        m_currentPlaylistTreeModel->setSourceTreeModel( ViewManager::instance()->treeModelForCollection( collection ) );
        m_view->rootContext()->setContextProperty( "currentPlaylistTreeModel", m_currentPlaylistTreeModel );
    }
}



void
TomahawkTouchWindow::loadQml()
{
    tLog() << Q_FUNC_INFO;
    qmlRegisterType<AudioEngine>("org.tomahawkplayer.qmlcomponents", 1, 0, "AudioEngine");

    if( !m_view )
    {
        tLog()<< Q_FUNC_INFO << "create qml view";
        m_view = new QDeclarativeView;
        m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
        m_view->show();
    }

    tLog()<< Q_FUNC_INFO << "clear component cache";
    m_view->engine()->clearComponentCache();

    tLog()<< Q_FUNC_INFO << "set context property";
    QDeclarativeContext* context = m_view->rootContext();

    tLog()<< Q_FUNC_INFO << "make objects accessible from qml";
    context->setContextProperty( "touchWindow", this );
    context->setContextProperty( "audioEngine", AudioEngine::instance() );
    context->setContextProperty( "globalActionManager", GlobalActionManager::instance() );
    context->setContextProperty( "sourcesModel", s_sourcesModel );
    context->setContextProperty( "currentPlaylistTreeModel", m_currentPlaylistTreeModel );

    tLog()<< Q_FUNC_INFO << "set source";
    m_view->setSource( QUrl::fromLocalFile( QMLGUI "/main.qml" ) );
}
