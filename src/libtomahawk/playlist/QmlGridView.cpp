/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Michael Zanetti <mzanetti@kde.org>
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

#include "QmlGridView.h"
#include "widgets/DeclarativeCoverArtProvider.h"
#include "PlayableProxyModelPlaylistInterface.h"
#include "ViewManager.h"
#include "audio/AudioEngine.h"

#include <QDeclarativeContext>

using namespace Tomahawk;

class QmlGridPlaylistInterface : public PlayableProxyModelPlaylistInterface
{
    Q_OBJECT
public:
    explicit QmlGridPlaylistInterface( PlayableProxyModel* proxy, QmlGridView* view ) : PlayableProxyModelPlaylistInterface( proxy ), m_view( view ) {}

    virtual bool hasChildInterface( playlistinterface_ptr playlistInterface )
    {
//        if ( m_view.isNull() || !m_view.data()->m_playing.isValid() )
//        {
//            return false;
//        }

//        PlayableItem* item = m_view.data()->model()->itemFromIndex( m_view.data()->proxyModel()->mapToSource( m_view.data()->m_playing ) );
//        if ( item )
//        {
//            if ( !item->album().isNull() )
//                return item->album()->playlistInterface( Tomahawk::Mixed ) == playlistInterface;
//            else if ( !item->artist().isNull() )
//                return item->artist()->playlistInterface( Tomahawk::Mixed ) == playlistInterface;
//            else if ( !item->query().isNull() && !playlistInterface.dynamicCast< SingleTrackPlaylistInterface >().isNull() )
//                return item->query() == playlistInterface.dynamicCast< SingleTrackPlaylistInterface >()->track();
//        }

        return false;
    }
private:
    QWeakPointer<QmlGridView> m_view;
};

QmlGridView::QmlGridView(QWidget *parent) : DeclarativeView(parent)
{
    m_proxyModel = new PlayableProxyModel( this );
    m_playlistInterface = playlistinterface_ptr( new QmlGridPlaylistInterface( m_proxyModel, this ) );

    rootContext()->setContextProperty( "mainModel", m_proxyModel );

    setSource( QUrl( "qrc" RESPATH "qml/QmlGridView.qml" ) );
}

QmlGridView::~QmlGridView()
{

}

void QmlGridView::setPlayableModel(PlayableModel *model)
{
//    m_inited = false;
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourcePlayableModel( m_model );
        m_proxyModel->sort( 0 );
    }

    emit modelChanged();

}


void
QmlGridView::onItemClicked( int index )
{
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( m_proxyModel->index( index, 0) ) );
    if ( item )
    {
        if ( !item->album().isNull() )
            ViewManager::instance()->show( item->album() );
        else if ( !item->artist().isNull() )
            ViewManager::instance()->show( item->artist() );
        else if ( !item->query().isNull() )
            ViewManager::instance()->show( item->query() );
    }
}

void QmlGridView::onItemPlayClicked(int index)
{
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( m_proxyModel->index( index, 0) ) );
    if ( item )
    {
        if ( !item->query().isNull() )
            AudioEngine::instance()->playItem( Tomahawk::playlistinterface_ptr(), item->query() );
        else if ( !item->album().isNull() )
            AudioEngine::instance()->playItem( item->album() );
        else if ( !item->artist().isNull() )
            AudioEngine::instance()->playItem( item->artist() );
    }
}
#include "QmlGridView.moc"
