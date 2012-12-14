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
#include "widgets/Breadcrumb.h"

#include <QUrl>
#include <QSize>
#include <QVBoxLayout>
#include <qdeclarative.h>
#include <QDeclarativeContext>

namespace Tomahawk
{

DynamicQmlWidget::DynamicQmlWidget( const dynplaylist_ptr& playlist, QWidget* parent )
    : QWidget( parent )
    , m_playlist( playlist )
    , m_runningOnDemand( false )
    , m_activePlaylist( false )
{
    m_model = new DynamicModel( this );

    m_proxyModel = new PlayableProxyModel( this );
    m_proxyModel->setSourcePlayableModel( m_model );
    m_proxyModel->setShowOfflineResults( false );

    m_model->loadPlaylist( m_playlist );

    m_artistChartsModel = new PlayableModel( this );


    QVBoxLayout *vLayout = new QVBoxLayout();
    setLayout(vLayout);
    TomahawkUtils::unmarginLayout(vLayout);

    createStationModel();
    m_breadcrumb = new Breadcrumb();
    connect(m_breadcrumb, SIGNAL(activateIndex(QModelIndex)), SLOT(breadcrumbChanged(QModelIndex)));
    m_breadcrumb->setModel(m_createStationModel);
    m_breadcrumb->setRootIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::Charts, TomahawkUtils::Original ) );
    vLayout->addWidget(m_breadcrumb);

    DeclarativeView *m_declarativeView = new DeclarativeView();
    vLayout->addWidget(m_declarativeView);
    qRegisterMetaType<CreateBy>("CreateBy");

    qmlRegisterUncreatableType<GeneratorInterface>("tomahawk", 1, 0, "Generator", "you cannot create it on your own - should be set in context");
    qmlRegisterUncreatableType<DynamicQmlWidget>("tomahawk", 1, 0, "StationWidget", "bla");

    m_declarativeView->rootContext()->setContextProperty( "dynamicModel", m_proxyModel );
    m_declarativeView->rootContext()->setContextProperty( "artistChartsModel", m_artistChartsModel );
    m_declarativeView->rootContext()->setContextProperty( "generator", m_playlist->generator().data() );
    m_declarativeView->rootContext()->setContextProperty( "stationView", this );

    m_declarativeView->setSource( QUrl( "qrc" RESPATH "qml/StationScene.qml" ) );

    connect( m_model, SIGNAL( currentItemChanged(QPersistentModelIndex)), SLOT( currentIndexChanged( QPersistentModelIndex ) ) );
    connect( m_model, SIGNAL( loadingStarted() ), SIGNAL(loadingChanged() ) );
    connect( m_model, SIGNAL( loadingFinished() ), SIGNAL(loadingChanged() ) );
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

bool DynamicQmlWidget::loading()
{
    return m_model->isLoading();
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
    m_playlist->generator()->startFromArtist(Artist::get(artist));
    emit configuredChanged();
}

void DynamicQmlWidget::startStationFromGenre(const QString &genre)
{
    tDebug() << "should start startion from genre" << genre;
    m_model->clear();
    m_playlist->generator()->startFromGenre( genre );
    emit configuredChanged();
}

void DynamicQmlWidget::currentIndexChanged( const QPersistentModelIndex &currentIndex )
{
    tDebug() << "current index is" << currentIndex.row();
    m_declarativeView->rootContext()->setContextProperty( "currentlyPlayedIndex", m_proxyModel->mapFromSource( currentIndex ).row() );
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

    QStandardItem *byArtistItem = 0;
    for(int i = 0; i < m_createStationModel->rowCount(); ++i) {
        QStandardItem *item = m_createStationModel->itemFromIndex(m_createStationModel->index(i, 0));
        tDebug() << "##" << item->data(Qt::UserRole);
        if(item->data(RoleCreateBy).toInt() == CreateByArtist) {
            byArtistItem = item;
            break;
        }
    }
    if(!byArtistItem) {
        // huh? no "...by artist selection? Cannot continue...
        return;
    }

    QStandardItem *selectArtistItem = new QStandardItem("choose artist...");
    selectArtistItem->setData(1, RolePage);
    selectArtistItem->setData(CreateByArtist, RoleCreateBy);
    byArtistItem->appendRow(selectArtistItem);
    foreach(const Tomahawk::artist_ptr &artist, artists) {
        QStandardItem *artistItem = new QStandardItem(artist->name());
        artistItem->setData(2, RolePage);
        artistItem->setData(CreateByNone, RoleCreateBy);
        byArtistItem->appendRow(artistItem);
    }
    m_breadcrumb->setModel(m_createStationModel);
}

void DynamicQmlWidget::createStationModel()
{
    m_createStationModel = new QStandardItemModel(this);

    // Create Station by...
    QStandardItem *selectionItem = new QStandardItem("Start...");
    selectionItem->setData(0, RolePage);
    selectionItem->setData(CreateByNone, RoleCreateBy);
    m_createStationModel->insertRow(0, selectionItem);

    QStandardItem *byArtistItem = new QStandardItem("...by Artist");
    byArtistItem->setData(1, RolePage);
    byArtistItem->setData(CreateByArtist, RoleCreateBy);
    m_createStationModel->insertRow(0, byArtistItem);

    QStandardItem *byGenreItem = new QStandardItem("...by Genre");
    byArtistItem->setData(1, RolePage);
    byGenreItem->setData(CreateByGenre, RoleCreateBy);
    m_createStationModel->insertRow(0, byGenreItem);

    QStandardItem *byYearItem = new QStandardItem("...by Year");
    byArtistItem->setData(1, RolePage);
    byYearItem->setData(CreateByYear, RoleCreateBy);
    m_createStationModel->insertRow(0, byYearItem);

    // Fill in genres (static for now)
    QStringList genres;
    genres << "acoustic" << "alternative" << "alternative rock" << "classic" << "folk" << "indie" << "pop" <<
              "rock" << "hip-hop" << "punk" << "grunge" << "indie" << "electronic" << "country" << "jazz" <<
              "psychodelic" << "soundtrack" << "reggae" << "house" << "drum and base";

    QStandardItem *selectGenreItem = new QStandardItem("choose genre...");
    selectGenreItem->setData(1, RolePage);
    selectGenreItem->setData(CreateByGenre, RoleCreateBy);
    byGenreItem->appendRow(selectGenreItem);
    foreach(const QString &genre, genres) {
        QStandardItem *genreItem = new QStandardItem(genre);
        genreItem->setData(2, RolePage);
        genreItem->setData(CreateByNone, RoleCreateBy);
        byGenreItem->appendRow(genreItem);
    }

    // Fill in years (static for now)
    QStringList years;
    years << "50" << "60" << "70" << "80" << "90";

    QStandardItem *selectYearItem = new QStandardItem("choose year...");
    selectYearItem->setData(1, RolePage);
    selectYearItem->setData(CreateByYear, RoleCreateBy);
    byYearItem->appendRow(selectYearItem);
    foreach(const QString &year, years) {
        QStandardItem *yearItem = new QStandardItem(year);
        yearItem->setData(2, RolePage);
        yearItem->setData(CreateByNone, RoleCreateBy);
        byYearItem->appendRow(yearItem);
    }
}

void DynamicQmlWidget::breadcrumbChanged(const QModelIndex &index)
{
    tDebug() << "**************************************" << index.data(RolePage).toInt() << index.data(RoleCreateBy);
    emit pagePicked(index.data(RolePage).toInt(), index.data(RoleCreateBy).toInt());
}
}
