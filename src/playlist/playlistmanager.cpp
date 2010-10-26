#include "playlistmanager.h"

#include "audioengine.h"
#include "collectionmodel.h"
#include "collectionflatmodel.h"
#include "collectionview.h"
#include "playlistmodel.h"
#include "playlistview.h"
#include "trackproxymodel.h"
#include "trackmodel.h"

#define FILTER_TIMEOUT 280

PlaylistManager::PlaylistManager( QObject* parent )
    : QObject( parent )
    , m_widget( new QStackedWidget() )
    , m_superCollectionModel( new CollectionModel() )
    , m_superCollectionFlatModel( new CollectionFlatModel() )
    , m_currentProxyModel( 0 )
    , m_currentModel( 0 )
    , m_currentMode( 0 )
    , m_superCollectionVisible( true )
{
    m_widget->setMinimumWidth( 620 );

    m_superCollectionViews << new CollectionView();
    m_superCollectionViews.first()->setModel( m_superCollectionFlatModel );
    m_widget->addWidget( m_superCollectionViews.first() );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );
}


PlaylistManager::~PlaylistManager()
{
    delete m_widget;
}


bool
PlaylistManager::show( const Tomahawk::playlist_ptr& playlist )
{
    unlinkPlaylist();

    if ( !m_views.contains( playlist ) )
    {
        QList<PlaylistView*> views;
        {
            PlaylistView* view = new PlaylistView();
            PlaylistModel* model = new PlaylistModel();
            view->setModel( model );
            views << view;

            m_currentProxyModel = view->proxyModel();
            m_currentModel = view->model();

            model->loadPlaylist( playlist );
            playlist->resolve();
        }

        m_views.insert( playlist, views );
        m_widget->addWidget( views.first() );
        m_widget->setCurrentWidget( views.first() );
    }
    else
    {
        QList<PlaylistView*> views = m_views.value( playlist );
        m_widget->setCurrentWidget( views.first() );
        m_currentProxyModel = views.first()->proxyModel();
        m_currentModel = views.first()->model();
    }

    m_superCollectionVisible = false;
    linkPlaylist();

    emit numSourcesChanged( APP->sourcelist().count() );

    return true;
}


bool
PlaylistManager::show( const Tomahawk::collection_ptr& collection )
{
    if ( m_superCollectionVisible )
    {
        if ( !m_superCollections.contains( collection ) )
        {
            m_superCollections.append( collection );
            m_superCollectionFlatModel->addCollection( collection );

            collection->loadAllTracks();
        }
        else
        {
            m_superCollectionFlatModel->removeCollection( collection );
            m_superCollections.removeAll( collection );
        }
    }

    unlinkPlaylist();

    if ( m_currentMode == 0 )
    {
        m_widget->setCurrentWidget( m_superCollectionViews.first() );
        m_currentProxyModel = m_superCollectionViews.first()->proxyModel();
        m_currentModel = m_superCollectionViews.first()->model();
    }
    else
    {
        m_widget->setCurrentWidget( m_superCollectionViews.at( 1 ) );
        m_currentProxyModel = m_superCollectionViews.at( 1 )->proxyModel();
        m_currentModel = m_superCollectionViews.at( 1 )->model();
    }

    m_superCollectionVisible = true;
    linkPlaylist();

    emit numSourcesChanged( m_superCollections.count() );

    return true;
}


void
PlaylistManager::setTreeMode()
{
    return;

    qDebug() << Q_FUNC_INFO;

    m_currentMode = 1;
    m_widget->setCurrentWidget( m_superCollectionViews.at( 1 ) );
}


void
PlaylistManager::setTableMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = 0;
    m_widget->setCurrentWidget( m_superCollectionViews.first() );
}


void
PlaylistManager::setFilter( const QString& filter )
{
    m_filter = filter;

    m_filterTimer.stop();
    m_filterTimer.setInterval( FILTER_TIMEOUT );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}


void
PlaylistManager::applyFilter()
{
    qDebug() << Q_FUNC_INFO;

    if ( m_currentProxyModel )
        m_currentProxyModel->setFilterRegExp( m_filter );
}


void
PlaylistManager::unlinkPlaylist()
{
    if ( m_currentModel )
    {
        disconnect( m_currentModel,      SIGNAL( trackCountChanged( unsigned int ) ),
                    this,                  SLOT( onTrackCountChanged( unsigned int ) ) );
    }

    if ( m_currentProxyModel )
    {
        disconnect( m_currentProxyModel, SIGNAL( trackCountChanged( unsigned int ) ),
                    this,                  SLOT( onTrackCountChanged( unsigned int ) ) );

        disconnect( m_currentProxyModel, SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
                    this,                SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

        disconnect( m_currentProxyModel, SIGNAL( shuffleModeChanged( bool ) ),
                    this,                SIGNAL( shuffleModeChanged( bool ) ) );
    }
}


void
PlaylistManager::linkPlaylist()
{
    connect( m_currentModel,      SIGNAL( trackCountChanged( unsigned int ) ),
             this,                  SLOT( onTrackCountChanged( unsigned int ) ) );

    connect( m_currentProxyModel, SIGNAL( trackCountChanged( unsigned int ) ),
             this,                  SLOT( onTrackCountChanged( unsigned int ) ) );

    connect( m_currentProxyModel, SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
             this,                SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

    connect( m_currentProxyModel, SIGNAL( shuffleModeChanged( bool ) ),
             this,                SIGNAL( shuffleModeChanged( bool ) ) );

    applyFilter();
    APP->audioEngine()->setPlaylist( (PlaylistInterface*)m_currentProxyModel );

    emit numTracksChanged( m_currentModel->trackCount() );
    emit repeatModeChanged( m_currentProxyModel->repeatMode() );
    emit shuffleModeChanged( m_currentProxyModel->shuffled() );
}


void
PlaylistManager::onTrackCountChanged( unsigned int )
{
    emit numTracksChanged( m_currentModel->trackCount() );
    emit numShownChanged( m_currentProxyModel->trackCount() );
}


void
PlaylistManager::setRepeatMode( PlaylistInterface::RepeatMode mode )
{
    if ( m_currentProxyModel )
        m_currentProxyModel->setRepeatMode( mode );
}


void
PlaylistManager::setShuffled( bool enabled )
{
    if ( m_currentProxyModel )
        m_currentProxyModel->setShuffled( enabled );
}
