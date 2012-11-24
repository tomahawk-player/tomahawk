#include "DynamicQmlWidget.h"

#include "playlist/dynamic/DynamicModel.h"
#include "playlist/PlayableProxyModel.h"
#include "utils/TomahawkUtilsGui.h"
#include "playlist/dynamic/DynamicModel.h"
#include "playlist/dynamic/echonest/EchonestControl.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/PlayableItem.h"
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


    qDebug() << "###got" << m_playlist->generator()->controls().size() << "controls";

    // TODO: In case QML is used in more places, this should probably be moved to some generic place
    qmlRegisterType<PlayableItem>("tomahawk", 1, 0, "PlayableItem");
    qmlRegisterUncreatableType<GeneratorInterface>("tomahawk", 1, 0, "Generator", "you cannot create it on your own - should be set in context");
    qmlRegisterUncreatableType<PlayableItem>("tomahawk", 1, 0, "PlayableItem", "you cannot create it on your own - they will appear in the model");


    rootContext()->setContextProperty( "dynamicModel", m_proxyModel );
    rootContext()->setContextProperty( "generator", m_playlist->generator().data() );
    rootContext()->setContextProperty( "rootView", this );

    setSource( QUrl( "qrc" RESPATH "qml/StationScene.qml" ) );

    connect( m_model, SIGNAL( currentItemChanged( QPersistentModelIndex ) ), SLOT( currentItemChanged( QPersistentModelIndex ) ) );
    connect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( nextTrackGenerated( Tomahawk::query_ptr ) ) );
    connect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_playlist->generator().data(), SIGNAL( error( QString, QString )), SLOT( error(QString,QString) ) );

    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), this, SLOT( trackStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( playlistChanged( Tomahawk::playlistinterface_ptr ) ), this, SLOT( playlistChanged( Tomahawk::playlistinterface_ptr ) ) );

    // Initially seed the playlist
    m_playlist->generator()->startFromArtist( Artist::get( "Eminem" , false ) );
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

playlist_ptr DynamicQmlWidget::playlist() const
{
    return m_model->playlist();
}

void DynamicQmlWidget::playItem(int index)
{
    qDebug() << "playItem called for cover" << index;
    AudioEngine::instance()->playItem( m_proxyModel->playlistInterface(), m_proxyModel->itemFromIndex( index )->result() );
}

void DynamicQmlWidget::pause()
{
    AudioEngine::instance()->pause();
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
