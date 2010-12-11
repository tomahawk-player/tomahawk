#include "playlistmanager.h"

#include <QVBoxLayout>

#include "audio/audioengine.h"
#include "utils/animatedsplitter.h"

#include "collectionmodel.h"
#include "collectionflatmodel.h"
#include "collectionview.h"
#include "playlistmodel.h"
#include "playlistview.h"
#include "queueview.h"
#include "trackproxymodel.h"
#include "trackmodel.h"
#include "albumview.h"
#include "albumproxymodel.h"
#include "albummodel.h"

#include "infowidgets/sourceinfowidget.h"
#include <widgets/DynamicWidget.h>

#define FILTER_TIMEOUT 280


PlaylistManager::PlaylistManager( QObject* parent )
    : QObject( parent )
    , m_widget( new QWidget() )
    , m_currentInterface( 0 )
    , m_currentMode( 0 )
    , m_superCollectionVisible( true )
    , m_statsAvailable( false )
    , m_modesAvailable( false )
{
    m_stack = new QStackedWidget();

    m_widget->setLayout( new QVBoxLayout() );

    m_splitter = new AnimatedSplitter();
    m_splitter->setOrientation( Qt::Vertical );
    m_splitter->setChildrenCollapsible( false );
    m_splitter->setGreedyWidget( 0 );
    m_splitter->addWidget( m_stack );

    m_queueView = new QueueView( m_splitter );
    m_queueModel = new PlaylistModel( m_queueView );
    m_queueView->queue()->setModel( m_queueModel );
    APP->audioEngine()->setQueue( m_queueView->queue()->proxyModel() );

    m_splitter->addWidget( m_queueView );
    m_splitter->hide( 1, false );

    m_widget->layout()->setMargin( 0 );
    m_widget->layout()->addWidget( m_splitter );

    m_superCollectionView = new CollectionView();
    m_superCollectionFlatModel = new CollectionFlatModel( m_superCollectionView );
    m_superCollectionView->setModel( m_superCollectionFlatModel );

    m_superAlbumView = new AlbumView();
    m_superAlbumModel = new AlbumModel( m_superAlbumView );
    m_superAlbumView->setModel( m_superAlbumModel );

    m_stack->addWidget( m_superCollectionView );
    m_stack->addWidget( m_superAlbumView );
    m_currentInterface = m_superCollectionView->proxyModel();
    
    m_playlistView = new PlaylistView();
    m_stack->addWidget( m_playlistView );
    m_playlistModel = new PlaylistModel();
    
    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );
}


PlaylistManager::~PlaylistManager()
{
    delete m_widget;
}


PlaylistView*
PlaylistManager::queue() const
{
    return m_queueView->queue();
}


bool
PlaylistManager::show( const Tomahawk::playlist_ptr& playlist )
{
    unlinkPlaylist();
    
    m_playlistModel->loadPlaylist( playlist );
    m_playlistView->setModel( m_playlistModel );
    
    if ( !m_loadedPlaylists.contains( playlist ) )
    {
        playlist->resolve();
        m_loadedPlaylists << playlist;
    }
    
    m_currentInterface = m_playlistView->proxyModel();
    m_stack->setCurrentWidget( m_playlistView );
    
    m_superCollectionVisible = false;
    m_statsAvailable = true;
    m_modesAvailable = false;
    linkPlaylist();

    emit numSourcesChanged( APP->sourcelist().count() );
    return true;
}

bool 
PlaylistManager::show(const Tomahawk::dynplaylist_ptr& playlist)
{
    unlinkPlaylist();
    
    if( !m_dynamicWidgets.contains( playlist ) ) {
       m_dynamicWidgets[ playlist ] = new Tomahawk::DynamicWidget( playlist, m_stack );
       m_stack->addWidget( m_dynamicWidgets[ playlist ] );
    }
    
    m_stack->setCurrentWidget( m_dynamicWidgets.value( playlist ) );
    m_currentInterface = m_dynamicWidgets.value( playlist )->playlistInterface();
    
    m_superCollectionVisible = false;
    m_statsAvailable = true;
    m_modesAvailable = false;
    linkPlaylist();
    
    emit numSourcesChanged( APP->sourcelist().count() );
    return true;
}


bool
PlaylistManager::show( const Tomahawk::album_ptr& album )
{
    qDebug() << Q_FUNC_INFO << &album << album.data();
    unlinkPlaylist();

    m_playlistModel->loadAlbum( album );
    m_stack->setCurrentWidget( m_playlistView );
    
    m_currentInterface = m_playlistView->proxyModel();
    
    m_superCollectionVisible = false;
    m_statsAvailable = false;
    m_modesAvailable = false;
    linkPlaylist();

    emit numSourcesChanged( 1 );
    return true;
}


bool
PlaylistManager::show( const Tomahawk::collection_ptr& collection )
{
    unlinkPlaylist();

    m_currentCollection = collection;
    if ( m_currentMode == 0 )
    {
        if ( !m_collectionViews.contains( collection ) )
        {
            CollectionView* view = new CollectionView();
            CollectionFlatModel* model = new CollectionFlatModel();
            view->setModel( model );
            model->addCollection( collection );

            m_currentInterface = view->proxyModel();
            m_collectionViews.insert( collection, view );

            m_stack->addWidget( view );
            m_stack->setCurrentWidget( view );
        }
        else
        {
            CollectionView* view = m_collectionViews.value( collection );
            m_stack->setCurrentWidget( view );
            m_currentInterface = view->proxyModel();
        }
    }

    if ( m_currentMode == 2 )
    {
        if ( !m_collectionAlbumViews.contains( collection ) )
        {
            AlbumView* aview = new AlbumView();
            AlbumModel* amodel = new AlbumModel( aview );
            aview->setModel( amodel );
            amodel->addCollection( collection );

            m_currentInterface = aview->proxyModel();
            m_collectionAlbumViews.insert( collection, aview );

            m_stack->addWidget( aview );
            m_stack->setCurrentWidget( aview );
        }
        else
        {
            AlbumView* view = m_collectionAlbumViews.value( collection );
            m_stack->setCurrentWidget( view );
            m_currentInterface = view->proxyModel();
        }
    }

    m_superCollectionVisible = false;
    m_statsAvailable = ( m_currentMode == 0 );
    m_modesAvailable = true;
    linkPlaylist();

    emit numSourcesChanged( 1 );
    return true;
}


bool
PlaylistManager::show( const Tomahawk::source_ptr& source )
{
    unlinkPlaylist();

    m_currentInterface = 0;

    if ( !m_sourceViews.contains( source ) )
    {
        SourceInfoWidget* swidget = new SourceInfoWidget( source );
        m_currentInfoWidget = swidget;
        m_stack->addWidget( m_currentInfoWidget );
        m_sourceViews.insert( source, swidget );
    }
    else
    {
        m_currentInfoWidget = m_sourceViews.value( source );
    }

    m_stack->setCurrentWidget( m_currentInfoWidget );
    m_superCollectionVisible = false;
    m_statsAvailable = false;
    m_modesAvailable = false;

    linkPlaylist();

    emit numSourcesChanged( 1 );
    return true;
}


bool
PlaylistManager::showSuperCollection()
{
    foreach( const Tomahawk::source_ptr& source, APP->sourcelist().sources() )
    {
        if ( !m_superCollections.contains( source->collection() ) )
        {
            m_superCollections.append( source->collection() );
            m_superCollectionFlatModel->addCollection( source->collection() );
            m_superAlbumModel->addCollection( source->collection() );
        }
    }

    if ( m_currentMode == 0 )
    {
        m_stack->setCurrentWidget( m_superCollectionView );
        m_currentInterface = m_superCollectionView->proxyModel();
    }
    else if ( m_currentMode == 2 )
    {
        m_stack->setCurrentWidget( m_superAlbumView );
        m_currentInterface = m_superAlbumView->proxyModel();
    }

    m_superCollectionVisible = true;
    m_statsAvailable = ( m_currentMode == 0 );
    m_modesAvailable = true;
    linkPlaylist();

    emit numSourcesChanged( m_superCollections.count() );
    return true;
}


void
PlaylistManager::setTableMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = 0;

    if ( m_superCollectionVisible )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
PlaylistManager::setTreeMode()
{
    return;

    qDebug() << Q_FUNC_INFO;

    m_currentMode = 1;

    if ( m_superCollectionVisible )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
PlaylistManager::setAlbumMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = 2;

    if ( m_superCollectionVisible )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
PlaylistManager::showQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "showQueue",
                                   Qt::QueuedConnection );
        return;
    }

    m_splitter->show( 1 );
}


void
PlaylistManager::hideQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "hideQueue",
                                   Qt::QueuedConnection );
        return;
    }

    m_splitter->hide( 1 );
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

    if ( m_currentInterface )
        m_currentInterface->setFilter( m_filter );
}


void
PlaylistManager::unlinkPlaylist()
{
    if ( m_currentInterface )
    {
        disconnect( m_currentInterface->object(), SIGNAL( sourceTrackCountChanged( unsigned int ) ),
                    this,                         SIGNAL( numTracksChanged( unsigned int ) ) );

        disconnect( m_currentInterface->object(), SIGNAL( trackCountChanged( unsigned int ) ),
                    this,                         SIGNAL( numShownChanged( unsigned int ) ) );

        disconnect( m_currentInterface->object(), SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
                    this,                         SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

        disconnect( m_currentInterface->object(), SIGNAL( shuffleModeChanged( bool ) ),
                    this,                         SIGNAL( shuffleModeChanged( bool ) ) );
    }
}


void
PlaylistManager::linkPlaylist()
{
    if ( m_currentInterface )
    {
        connect( m_currentInterface->object(), SIGNAL( sourceTrackCountChanged( unsigned int ) ),
                 this,                         SIGNAL( numTracksChanged( unsigned int ) ) );

        connect( m_currentInterface->object(), SIGNAL( trackCountChanged( unsigned int ) ),
                 this,                         SIGNAL( numShownChanged( unsigned int ) ) );

        connect( m_currentInterface->object(), SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
                 this,                         SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

        connect( m_currentInterface->object(), SIGNAL( shuffleModeChanged( bool ) ),
                 this,                         SIGNAL( shuffleModeChanged( bool ) ) );
    }

    applyFilter();
    APP->audioEngine()->setPlaylist( m_currentInterface );

    if ( m_currentInterface && m_statsAvailable )
    {
        emit numTracksChanged( m_currentInterface->unfilteredTrackCount() );
        emit numShownChanged( m_currentInterface->trackCount() );
        emit repeatModeChanged( m_currentInterface->repeatMode() );
        emit shuffleModeChanged( m_currentInterface->shuffled() );

    }

    emit statsAvailable( m_statsAvailable );
    emit modesAvailable( m_modesAvailable );
}


void
PlaylistManager::setRepeatMode( PlaylistInterface::RepeatMode mode )
{
    if ( m_currentInterface )
        m_currentInterface->setRepeatMode( mode );
}


void
PlaylistManager::setShuffled( bool enabled )
{
    if ( m_currentInterface )
        m_currentInterface->setShuffled( enabled );
}


void
PlaylistManager::showCurrentTrack()
{
    unlinkPlaylist();

    m_currentInterface = APP->audioEngine()->currentTrackPlaylist();

    if ( m_currentInterface->widget() )
        m_stack->setCurrentWidget( m_currentInterface->widget() );

    linkPlaylist();

/*    if ( m_currentView && m_currentProxyModel )
        m_currentView->scrollTo( m_currentProxyModel->currentItem(), QAbstractItemView::PositionAtCenter );*/
}
