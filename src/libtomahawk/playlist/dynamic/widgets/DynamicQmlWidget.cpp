#include "DynamicQmlWidget.h"

#include "playlist/dynamic/DynamicModel.h"
#include "playlist/PlayableProxyModel.h"
#include "utils/TomahawkUtilsGui.h"
#include "dynamic/DynamicModel.h"
#include "dynamic/echonest/EchonestControl.h"
#include "dynamic/GeneratorInterface.h"
#include "PlayableItem.h"
#include "Source.h"

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
    m_proxyModel->setShowOfflineResults( true );

    m_model->loadPlaylist( m_playlist );

    // Initially seed the playlist
    m_playlist->generator()->generate( 20 );

    qDebug() << "###got" << m_playlist->generator()->controls().size() << "controls";

    qmlRegisterUncreatableType<EchonestStation>("tomahawk", 1, 0, "EchonestStation", "bla");

    // TODO: In case QML is used in more places, this should probably be moved to some generic place
    qmlRegisterType<PlayableItem>("tomahawk", 1, 0, "PlayableItem");
//    qmlRegisterUncreatableType<Tomahawk::DynamicControl>("tomahawk", 1, 0, "DynamicControl", "use generator.createControl() isntead");
//    qmlRegisterUncreatableType<Tomahawk::EchonestControl>("tomahawk", 1, 0, "EchonestControl", "use Generator.createControl() instead");
    qmlRegisterUncreatableType<DynamicControl>("tomahawk", 1, 0, "DynamicControl", "use generator.createControl() isntead");
    qmlRegisterUncreatableType<EchonestControl>("tomahawk", 1, 0, "EchonestControl", "use Generator.createControl() instead");
    qmlRegisterUncreatableType<GeneratorInterface>("tomahawk", 1, 0, "Generator", "you cannot create it on your own - should be set in context");

    QStringList generatorControls;

    foreach(dyncontrol_ptr control, m_playlist->generator()->controls()) {
        qDebug() << "**CTRL" << control->summary() << control->input() << control->match() << control->type() << control->selectedType();
        generatorControls << control->summary();
    }

    ControlModel *controls = new ControlModel(m_playlist->generator(), this);

    EchonestStation *station = new EchonestStation(m_playlist->generator(), this);
    rootContext()->setContextProperty( "echonestStation", station);
    rootContext()->setContextProperty( "controlModel", controls );
    rootContext()->setContextProperty( "dynamicModel", m_proxyModel );
    rootContext()->setContextProperty( "generator", m_playlist->generator().data() );
    currentItemChanged( m_model->currentItem() );

    setSource( QUrl( "qrc" RESPATH "qml/StationScene.qml" ) );

    connect( m_model, SIGNAL( currentItemChanged( QPersistentModelIndex ) ), SLOT( currentItemChanged( QPersistentModelIndex ) ) );
    connect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    connect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );

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

    QModelIndex index = m_proxyModel->mapToSource( m_proxyModel->index( id.toInt(), 0, QModelIndex() ) );
    qDebug() << "!*!*!*! got index" << index << id;
    if( index.isValid() ) {
        PlayableItem *item = m_model->itemFromIndex( index );
        qDebug() << "item:" << item;
        qDebug() << "item2:" << item->artistName() << item->name();
        if ( !item->query().isNull() ) {
            return item->query()->displayQuery()->cover( *size );
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

void
DynamicQmlWidget::tracksGenerated( const QList< query_ptr >& queries )
{
    qDebug() << queries.count() << "tracks generated";
    m_model->tracksGenerated( queries, queries.count() );
    m_playlist->resolve();

    // Ok... we have some intial stuff, switch to dynamic mode
    //m_model->startOnDemand();
}

void DynamicQmlWidget::onRevisionLoaded(DynamicPlaylistRevision)
{
    m_playlist->resolve();
}

}
