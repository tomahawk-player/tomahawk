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
#include "database/DatabaseCommand_CreateDynamicPlaylist.h"
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

    connect( m_model, SIGNAL( currentIndexChanged()), SLOT( currentIndexChanged() ) );
    connect( m_model, SIGNAL( loadingStarted() ), SIGNAL(loadingChanged() ) );
    connect( m_model, SIGNAL( loadingFinished() ), SIGNAL(loadingChanged() ) );
    connect( m_model, SIGNAL( changed() ), SIGNAL( titleChanged() ) );
    connect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( nextTrackGenerated( Tomahawk::query_ptr ) ) );
    connect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_playlist->generator().data(), SIGNAL( error( QString, QString )), SLOT( error(QString,QString) ) );

    if (configured()) {
        m_playlist->generator()->generate( 20 );
    } else {
        // TODO: only load if needed, i.e. the user clicks on start station by artist
        loadArtistCharts();
    }
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
    if ( !m_playlist->title().isEmpty() ) {
        return m_playlist->title();
    }
    return "Listen to radio...";
}


void
DynamicQmlWidget::setTitle( const QString& title )
{
    m_model->setTitle( title );
    m_playlist->setTitle( title );
    m_model->playlist()->setTitle( title );

    if ( !m_playlist->loaded() )
    {
        tDebug() << "CONTROLS ARE SAVED:" << m_playlist->generator()->controls();
        DatabaseCommand_CreateDynamicPlaylist* cmd = new DatabaseCommand_CreateDynamicPlaylist( SourceList::instance()->getLocal(), m_playlist, true );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

        m_playlist->reportCreated( m_playlist );
        m_playlist->createNewRevision( uuid(), m_playlist->currentrevision(), m_playlist->type(), m_playlist->generator()->controls() );

        emit titleChanged();
    }
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
//    return true;
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

void DynamicQmlWidget::startStationFromYear(int year)
{
    tDebug() << "should start startion from year" << year;
    m_model->clear();
    m_playNextResolved = true;
    m_playlist->generator()->startFromYear( year );
    emit loadingChanged();
    emit configuredChanged();
}

void DynamicQmlWidget::startStationFromTo(int yearFrom, int yearTo)
{
    tDebug() << "should start startion from years" << yearFrom << "to" << yearTo;
    m_model->clear();
    m_playNextResolved = true;
    m_playlist->generator()->startFromTo( yearFrom, yearTo );
    emit loadingChanged();
    emit configuredChanged();
}

void DynamicQmlWidget::currentIndexChanged()
{
    tDebug() << "current index is" << m_model->currentItem().row();
    rootContext()->setContextProperty( "currentlyPlayedIndex", m_proxyModel->mapFromSource( m_model->currentItem() ).row() );
}

void
DynamicQmlWidget::tracksGenerated( const QList< query_ptr >& queries )
{
    m_model->tracksGenerated( queries, queries.count() );
    m_playlist->resolve();
}

void DynamicQmlWidget::nextTrackGenerated(const query_ptr &track)
{
    tDebug() << Q_FUNC_INFO << track->toString();
    m_model->tracksGenerated( QList<query_ptr>() << track );
    m_playlist->resolve();

    connect( track.data(), SIGNAL( resolvingFinished( bool )), SLOT( resolvingFinished( bool ) ) );

}

void DynamicQmlWidget::error(const QString &title, const QString &body)
{
    tDebug() << "got a generator error:" << title << body;
}

void DynamicQmlWidget::onRevisionLoaded(DynamicPlaylistRevision)
{
    m_playlist->resolve();
}

void DynamicQmlWidget::resolvingFinished(bool hasResults)
{
    Q_UNUSED(hasResults)
    tDebug() << "next track generated" << m_proxyModel->rowCount() << m_proxyModel->currentIndex().row();
    if( m_proxyModel->rowCount() <= m_proxyModel->currentIndex().row() + 8 ) {
        tDebug() << "fetching next one";
        m_playlist->generator()->fetchNext();
    }

    if( m_playNextResolved && m_proxyModel->rowCount() > 0 ) {
        playItem( 0 );
        m_playNextResolved = false;
    }
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
