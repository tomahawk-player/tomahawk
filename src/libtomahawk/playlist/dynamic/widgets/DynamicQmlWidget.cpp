#include "DynamicQmlWidget.h"

#include "playlist/dynamic/DynamicModel.h"
#include "playlist/PlayableProxyModel.h"
#include "playlist/dynamic/DynamicModel.h"
#include "playlist/dynamic/echonest/EchonestControl.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/PlayableItem.h"
#include "Source.h"
#include "SourceList.h"
#include "audio/AudioEngine.h"
#include "database/Database.h"
#include "database/DatabaseCommand_PlaybackCharts.h"
#include "widgets/DeclarativeCoverArtProvider.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QUrl>
#include <QSize>
#include <qdeclarative.h>
#include <QDeclarativeContext>

namespace Tomahawk
{

DynamicQmlWidget::DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent )
    : DeclarativeView( parent )
    , m_playlist( playlist )
    , m_runningOnDemand( false )
    , m_activePlaylist( false )
    , m_playNextResolved( false )
{
    m_model = new DynamicModel( this );

    m_proxyModel = new PlayableProxyModel( this );
    m_proxyModel->setSourcePlayableModel( m_model );
    m_proxyModel->setShowOfflineResults( false );

    m_model->loadPlaylist( m_playlist );

    m_artistChartsModel = new PlayableModel( this );


    qmlRegisterUncreatableType<GeneratorInterface>("tomahawk", 1, 0, "Generator", "you cannot create it on your own - should be set in context");

    rootContext()->setContextProperty( "dynamicModel", m_proxyModel );
    rootContext()->setContextProperty( "artistChartsModel", m_artistChartsModel );
    rootContext()->setContextProperty( "generator", m_playlist->generator().data() );
    rootContext()->setContextProperty( "currentlyPlayedIndex", QVariant::fromValue( 0 ) );

    setSource( QUrl( "qrc" RESPATH "qml/StationView.qml" ) );

    connect( m_model, SIGNAL( currentItemChanged(QPersistentModelIndex)), SLOT( currentIndexChanged( QPersistentModelIndex ) ) );
    connect( m_model, SIGNAL( loadingStarted() ), SIGNAL(loadingChanged() ) );
    connect( m_model, SIGNAL( loadingFinished() ), SIGNAL(loadingChanged() ) );
    connect( m_model, SIGNAL( changed() ), SIGNAL( titleChanged() ) );
    connect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( nextTrackGenerated( Tomahawk::query_ptr ) ) );
    connect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_playlist->generator().data(), SIGNAL( error( QString, QString )), SLOT( error(QString,QString) ) );

    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), this, SLOT( trackStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( playlistChanged( Tomahawk::playlistinterface_ptr ) ), this, SLOT( playlistChanged( Tomahawk::playlistinterface_ptr ) ) );

    //    m_playlist->generator()->generate( 20 );
    loadArtistCharts();
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


QString
DynamicQmlWidget::iconSource() const
{
    return QLatin1String( RESPATH "images/station.png" );
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

bool DynamicQmlWidget::loading()
{
    // Why does isLoading() not reset to true when cleared and station started again?
//    return m_model->isLoading();
    return m_playNextResolved && m_proxyModel->rowCount() == 0;
}

bool DynamicQmlWidget::configured()
{
    return !m_playlist->generator()->controls().isEmpty();
}

void DynamicQmlWidget::playItem(int index)
{
    tDebug() << "playItem called for cover" << index;
    AudioEngine::instance()->playItem( m_proxyModel->playlistInterface(), m_proxyModel->itemFromIndex( index )->result() );
}

void DynamicQmlWidget::pause()
{
    AudioEngine::instance()->pause();
}

void DynamicQmlWidget::startStationFromArtist(const QString &artist)
{
    m_model->clear();
    m_playNextResolved = true;
    m_playlist->generator()->startFromArtist(Artist::get(artist));
    emit loadingChanged();
    emit configuredChanged();
}

void DynamicQmlWidget::startStationFromGenre(const QString &genre)
{
    tDebug() << "should start startion from genre" << genre;
    m_model->clear();
    m_playNextResolved = true;
    m_playlist->generator()->startFromGenre( genre );
    emit loadingChanged();
    emit configuredChanged();
}

void DynamicQmlWidget::currentIndexChanged( const QPersistentModelIndex &currentIndex )
{
    tDebug() << "current index is" << currentIndex.row();
    rootContext()->setContextProperty( "currentlyPlayedIndex", m_proxyModel->mapFromSource( currentIndex ).row() );
}

void
DynamicQmlWidget::tracksGenerated( const QList< query_ptr >& queries )
{
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

    if( m_playNextResolved && m_proxyModel->rowCount() > 0 ) {
        playItem( 0 );
        m_playNextResolved = false;
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


void
DynamicQmlWidget::loadArtistCharts()
{
    DatabaseCommand_PlaybackCharts* cmd = new DatabaseCommand_PlaybackCharts( SourceList::instance()->getLocal(), this );
    cmd->setLimit( 15 );
    connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ), SLOT( onArtistCharts( QList< Tomahawk::artist_ptr > ) ), Qt::UniqueConnection );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DynamicQmlWidget::onArtistCharts( const QList< Tomahawk::artist_ptr >& artists )
{
    m_artistChartsModel->clear();
    m_artistChartsModel->appendArtists( artists );

}
}
