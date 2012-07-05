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
#include <QSize>

namespace Tomahawk
{

DynamicQmlWidget::DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent )
    : QDeclarativeView( parent )
    , QDeclarativeImageProvider( QDeclarativeImageProvider::Pixmap )
    , m_playlist( playlist )
{

    engine()->addImageProvider( "albumart", this );

    setResizeMode( QDeclarativeView::SizeRootObjectToView );

    m_model = new DynamicModel( this );
    m_proxyModel = new PlayableProxyModel( this );
    m_proxyModel->setSourcePlayableModel( m_model );
    m_proxyModel->setShowOfflineResults( false );

    m_model->loadPlaylist( m_playlist );
    m_model->startOnDemand();

    rootContext()->setContextProperty( "dynamicModel", m_proxyModel );
    currentItemChanged( m_model->currentItem() );

    // TODO: In case QML is used in more places, this should probably be moved to some generic place
    qmlRegisterType<PlayableItem>("tomahawk", 1, 0, "PlayableItem");

    setSource( QUrl( "qrc" RESPATH "qml/StationScene.qml" ) );

    connect( m_model, SIGNAL( currentItemChanged( QPersistentModelIndex ) ), SLOT( currentItemChanged( QPersistentModelIndex ) ) );
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

QPixmap DynamicQmlWidget::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    // We always can generate it in the requested size
    int width = requestedSize.width() > 0 ? requestedSize.width() : 230;
    int height = requestedSize.height() > 0 ? requestedSize.height() : 230;

    if( size )
        *size = QSize( width, height );

    QModelIndex index = m_model->index( id.toInt(), 0, QModelIndex() );
    qDebug() << "!*!*!*! got index" << index << id;
    if( index.isValid() ) {
        PlayableItem *item = m_model->itemFromIndex( index );
        qDebug() << "item:" << item;
        qDebug() << "item2:" << item->artistName() << item->name();
        if ( !item->album().isNull() ) {
            return item->album()->cover( *size );
        } else if ( !item->artist().isNull() ) {
            return item->artist()->cover( *size );
        } else if ( !item->query().isNull() ) {
            return item->query()->cover( *size );
        }
    }

    // TODO: create default cover art image
    QPixmap pixmap( *size );
    pixmap.fill();

    return pixmap;
}

void DynamicQmlWidget::currentItemChanged( const QPersistentModelIndex &currentIndex )
{
    rootContext()->setContextProperty( "currentlyPlayedIndex", m_proxyModel->mapFromSource( currentIndex ).row() );
}

}
