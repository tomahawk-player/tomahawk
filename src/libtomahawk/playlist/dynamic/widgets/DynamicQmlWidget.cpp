#include "DynamicQmlWidget.h"

#include "playlist/dynamic/DynamicModel.h"
#include "playlist/PlayableProxyModel.h"
#include "utils/TomahawkUtilsGui.h"

#include <QUrl>

namespace Tomahawk
{

DynamicQmlWidget::DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent )
    : QDeclarativeView( parent )
{
    setResizeMode(QDeclarativeView::SizeRootObjectToView);

    setSource(QUrl("qrc" RESPATH "qml/ArtistInfoScene.qml"));
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
