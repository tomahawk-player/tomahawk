#include "DynamicQmlWidget.h"
#include "utils/TomahawkUtilsGui.h"

#include <QUrl>

namespace Tomahawk
{

DynamicQmlWidget::DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent)
    : QDeclarativeView( parent )
{
    setResizeMode(QDeclarativeView::SizeRootObjectToView);

    setSource(QUrl("qrc" RESPATH "qml/ArtistInfoScene.qml"));


}

DynamicQmlWidget::~DynamicQmlWidget()
{

}

}
