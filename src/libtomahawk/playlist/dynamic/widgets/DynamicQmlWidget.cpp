#include "DynamicQmlWidget.h"

#include "playlist/dynamic/DynamicModel.h"
#include "playlist/PlayableProxyModel.h"
#include "utils/TomahawkUtilsGui.h"
#include "dynamic/DynamicModel.h"
#include "PlayableItem.h"

#include <QUrl>
#include <qdeclarative.h>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>

namespace Tomahawk
{

DynamicQmlWidget::DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent )
    : QDeclarativeView( parent )
    , m_playlist( playlist )
{
    setResizeMode( QDeclarativeView::SizeRootObjectToView );
    setSource( QUrl( "qrc" RESPATH "qml/ArtistInfoScene.qml" ) );

    m_model = new DynamicModel( this );
    m_proxyModel = new PlayableProxyModel( this );
    m_proxyModel->setSourcePlayableModel( m_model );

    rootContext()->setContextProperty( "dynamicModel", m_model );

    m_model->loadPlaylist( m_playlist );
    m_model->startOnDemand();

    setSource( QUrl( "qrc" RESPATH "qml/StationScene.qml" ) );
    // TODO: fill m_model with the station stuff
}


DynamicQmlWidget::~DynamicQmlWidget()
{
}


Tomahawk::playlistinterface_ptr
DynamicQmlWidget::playlistInterface() const
{
    return m_proxyModel->playlistInterface();
}


QString
DynamicQmlWidget::title() const
{
    return m_model->title();
}


QString
DynamicQmlWidget::description() const
{
    return m_model->description();
}


QPixmap
DynamicQmlWidget::pixmap() const
{
    return QPixmap( RESPATH "images/station.png" );
}


bool
DynamicQmlWidget::jumpToCurrentTrack()
{
    return false;
}

}
