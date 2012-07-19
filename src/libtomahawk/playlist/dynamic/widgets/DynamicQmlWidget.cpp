#include "DynamicQmlWidget.h"

#include "dynamic/echonest/EchonestStation.h"
#include "playlist/dynamic/DynamicModel.h"
#include "playlist/PlayableProxyModel.h"
#include "utils/TomahawkUtilsGui.h"
#include "dynamic/DynamicModel.h"
#include "dynamic/echonest/EchonestControl.h"
#include "dynamic/GeneratorInterface.h"
#include "PlayableItem.h"
#include "Source.h"
#include "widgets/DeclarativeCoverArtProvider.h"
#include "audio/AudioEngine.h"

#include <QUrl>
#include <qdeclarative.h>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QSize>

namespace Tomahawk
{

DynamicQmlWidget::DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent )
    : QDeclarativeView( parent )
    , m_playlist( playlist )
    , m_runningOnDemand( false )
    , m_activePlaylist( false )
{



    setResizeMode( QDeclarativeView::SizeRootObjectToView );

    m_model = new DynamicModel( this );

    m_proxyModel = new PlayableProxyModel( this );
    m_proxyModel->setSourcePlayableModel( m_model );
    m_proxyModel->setShowOfflineResults( false );

    // QML image providers will be deleted by the view
    engine()->addImageProvider( "albumart", new DeclarativeCoverArtProvider( m_proxyModel ) );

    m_model->loadPlaylist( m_playlist );
//    m_model->changeStation();
//    m_model->startOnDemand();

    // Initially seed the playlist
//    playlist->generator()->fetchNext();
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
    qmlRegisterUncreatableType<PlayableItem>("tomahawk", 1, 0, "PlayableItem", "you cannot create it on your own - they will appear in the model");

    QStringList generatorControls;

    foreach(dyncontrol_ptr control, m_playlist->generator()->controls()) {
        qDebug() << "**CTRL" << control->summary() << control->input() << control->match() << control->type() << control->selectedType();
        generatorControls << control->summary();
    }

    ControlModel *controls = new ControlModel(m_playlist->generator(), this);

    EchonestStation *station = new EchonestStation( m_proxyModel, m_playlist, this);
    rootContext()->setContextProperty( "echonestStation", station);
    rootContext()->setContextProperty( "controlModel", controls );
    rootContext()->setContextProperty( "dynamicModel", m_proxyModel );
    rootContext()->setContextProperty( "generator", m_playlist->generator().data() );
    currentItemChanged( m_model->currentItem() );

    setSource( QUrl( "qrc" RESPATH "qml/StationScene.qml" ) );

    connect( m_model, SIGNAL( currentItemChanged( QPersistentModelIndex ) ), SLOT( currentItemChanged( QPersistentModelIndex ) ) );
    connect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( nextTrackGenerated( Tomahawk::query_ptr ) ) );
    connect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_playlist->generator().data(), SIGNAL( error( QString, QString )), SLOT( error(QString,QString) ) );

    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), this, SLOT( trackStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( playlistChanged( Tomahawk::playlistinterface_ptr ) ), this, SLOT( playlistChanged( Tomahawk::playlistinterface_ptr ) ) );

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
    return true;
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
}

void DynamicQmlWidget::nextTrackGenerated(const query_ptr &track)
{
    m_model->tracksGenerated( QList<query_ptr>() << track );
    m_playlist->resolve();

    connect( track.data(), SIGNAL( resolvingFinished( bool )), SLOT( resolvingFinished( bool ) ) );

}

void DynamicQmlWidget::error(const QString &title, const QString &body)
{
    qDebug() << "got a generator error:" << title << body;

//    m_playlist->generator()->fetchNext();

}

void DynamicQmlWidget::onRevisionLoaded(DynamicPlaylistRevision)
{
    m_playlist->resolve();
}

void DynamicQmlWidget::resolvingFinished(bool hasResults)
{
    Q_UNUSED(hasResults)
    qDebug() << "next track generated" << m_proxyModel->rowCount() << m_proxyModel->currentIndex().row();
    if( m_proxyModel->rowCount() <= m_proxyModel->currentIndex().row() + 8 ) {
        qDebug() << "fetching next one";
        m_playlist->generator()->fetchNext();
    }
}

void DynamicQmlWidget::trackStarted()
{
    if ( m_activePlaylist && !m_playlist.isNull() &&
        m_playlist->mode() == OnDemand && !m_runningOnDemand )
    {

        startStation();
    }
}

void
DynamicQmlWidget::playlistChanged( Tomahawk::playlistinterface_ptr pl )
{
    if ( pl == m_proxyModel->playlistInterface() ) // same playlist
        m_activePlaylist = true;
    else
    {
        m_activePlaylist = false;

        // user started playing something somewhere else, so give it a rest
        if ( m_runningOnDemand )
        {
            stopStation( false );
        }
    }
}

void
DynamicQmlWidget::stopStation( bool stopPlaying )
{
    m_model->stopOnDemand( stopPlaying );
    m_runningOnDemand = false;

}

void
DynamicQmlWidget::startStation()
{
    m_runningOnDemand = true;
    m_model->startOnDemand();
}


}
