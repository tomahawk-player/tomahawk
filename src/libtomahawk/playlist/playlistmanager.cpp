#include "playlistmanager.h"

#include <QVBoxLayout>

#include "audio/audioengine.h"
#include "utils/animatedsplitter.h"
#include "infobar/infobar.h"
#include "topbar/topbar.h"
#include "widgets/infowidgets/sourceinfowidget.h"
#include "widgets/welcomewidget.h"

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
#include "sourcelist.h"
#include "tomahawksettings.h"

#include "dynamic/widgets/DynamicWidget.h"

#include "widgets/welcomewidget.h"
#include "widgets/infowidgets/sourceinfowidget.h"

#define FILTER_TIMEOUT 280

PlaylistManager* PlaylistManager::s_instance = 0;


PlaylistManager*
PlaylistManager::instance()
{
    return s_instance;
}


PlaylistManager::PlaylistManager( QObject* parent )
    : QObject( parent )
    , m_widget( new QWidget() )
    , m_welcomeWidget( new WelcomeWidget() )
    , m_currentInterface( 0 )
    , m_currentMode( 0 )
    , m_superCollectionVisible( true )
    , m_statsAvailable( false )
    , m_modesAvailable( false )
{
    s_instance = this;

    m_widget->setLayout( new QVBoxLayout() );

    m_topbar = new TopBar();
    m_infobar = new InfoBar();
    m_stack = new QStackedWidget();

    QFrame* line = new QFrame();
    line->setFrameStyle( QFrame::HLine );
    line->setStyleSheet( "border: 1px solid gray;" );
    line->setMaximumHeight( 1 );

    m_splitter = new AnimatedSplitter();
    m_splitter->setOrientation( Qt::Vertical );
    m_splitter->setChildrenCollapsible( false );
    m_splitter->setGreedyWidget( 0 );
    m_splitter->addWidget( m_stack );

    m_queueView = new QueueView( m_splitter );
    m_queueModel = new PlaylistModel( m_queueView );
    m_queueView->queue()->setModel( m_queueModel );
    AudioEngine::instance()->setQueue( m_queueView->queue()->proxyModel() );

    m_splitter->addWidget( m_queueView );
    m_splitter->hide( 1, false );

    m_widget->layout()->addWidget( m_infobar );
    m_widget->layout()->addWidget( m_topbar );
    m_widget->layout()->addWidget( line );
    m_widget->layout()->addWidget( m_splitter );

    m_superCollectionView = new CollectionView();
    m_superCollectionFlatModel = new CollectionFlatModel( m_superCollectionView );
    m_superCollectionView->setModel( m_superCollectionFlatModel );
    m_superCollectionView->setFrameShape( QFrame::NoFrame );
    m_superCollectionView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    m_superAlbumView = new AlbumView();
    m_superAlbumModel = new AlbumModel( m_superAlbumView );
    m_superAlbumView->setModel( m_superAlbumModel );

    m_stack->addWidget( m_superCollectionView );
    m_stack->addWidget( m_superAlbumView );

    m_currentInterface = m_superCollectionView->proxyModel();
    
    m_stack->setContentsMargins( 0, 0, 0, 0 );
    m_widget->setContentsMargins( 0, 0, 0, 0 );
    m_widget->layout()->setContentsMargins( 0, 0, 0, 0 );
    m_widget->layout()->setMargin( 0 );
    m_widget->layout()->setSpacing( 0 );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );

    connect( m_topbar, SIGNAL( filterTextChanged( QString ) ),
             this,       SLOT( setFilter( QString ) ) );

    connect( this,     SIGNAL( numSourcesChanged( unsigned int ) ),
             m_topbar,   SLOT( setNumSources( unsigned int ) ) );

    connect( this,     SIGNAL( numTracksChanged( unsigned int ) ),
             m_topbar,   SLOT( setNumTracks( unsigned int ) ) );

    connect( this,     SIGNAL( numArtistsChanged( unsigned int ) ),
             m_topbar,   SLOT( setNumArtists( unsigned int ) ) );

    connect( this,     SIGNAL( numShownChanged( unsigned int ) ),
             m_topbar,   SLOT( setNumShown( unsigned int ) ) );

    connect( m_topbar, SIGNAL( flatMode() ),
             this,       SLOT( setTableMode() ) );

    connect( m_topbar, SIGNAL( artistMode() ),
             this,       SLOT( setTreeMode() ) );

    connect( m_topbar, SIGNAL( albumMode() ),
             this,       SLOT( setAlbumMode() ) );

    connect( this,     SIGNAL( statsAvailable( bool ) ),
             m_topbar,   SLOT( setStatsVisible( bool ) ) );

    connect( this,     SIGNAL( modesAvailable( bool ) ),
             m_topbar,   SLOT( setModesVisible( bool ) ) );
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

    if ( !m_playlistViews.contains( playlist ) )
    {
        PlaylistView* view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setModel( model );
        view->setFrameShape( QFrame::NoFrame );
        view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        model->loadPlaylist( playlist );
        playlist->resolve();

        m_currentInterface = view->proxyModel();
        m_playlistViews.insert( playlist, view );

        m_stack->addWidget( view );
        m_stack->setCurrentWidget( view );
    }
    else
    {
        PlaylistView* view = m_playlistViews.value( playlist );
        m_stack->setCurrentWidget( view );
        m_currentInterface = view->proxyModel();
    }
    
    m_queueView->show();
    m_infobar->setCaption( playlist->title() );
    m_infobar->setDescription( tr( "A playlist by %1" ).arg( playlist->author()->isLocal() ? tr( "you" ) : playlist->author()->friendlyName() ) );

    m_superCollectionVisible = false;
    m_statsAvailable = true;
    m_modesAvailable = false;
    linkPlaylist();

    TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( playlist );

    emit numSourcesChanged( SourceList::instance()->count() );
    return true;
}


bool 
PlaylistManager::show( const Tomahawk::dynplaylist_ptr& playlist )
{
    unlinkPlaylist();
    
    if( !m_dynamicWidgets.contains( playlist ) )
    {
       m_dynamicWidgets[ playlist ] = new Tomahawk::DynamicWidget( playlist, m_stack );
       m_stack->addWidget( m_dynamicWidgets[ playlist ] );
       playlist->resolve();
    }
    
    m_stack->setCurrentWidget( m_dynamicWidgets.value( playlist ) );
    m_currentInterface = m_dynamicWidgets.value( playlist )->playlistInterface();

    m_infobar->setCaption( playlist->title() );
    m_infobar->setDescription( tr( "A playlist by %1" ).arg( playlist->author()->isLocal() ? tr( "you" ) : playlist->author()->friendlyName() ) );
    
    if( playlist->mode() == Tomahawk::OnDemand )
        m_queueView->hide();
    
    
    m_superCollectionVisible = false;
    m_statsAvailable = true;
    m_modesAvailable = false;
    linkPlaylist();
    
    TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( playlist );

    emit numSourcesChanged( SourceList::instance()->count() );

    return true;
}


bool
PlaylistManager::show( const Tomahawk::artist_ptr& artist )
{
    qDebug() << Q_FUNC_INFO << &artist << artist.data();
    unlinkPlaylist();

    if ( !m_artistViews.contains( artist ) )
    {
        PlaylistView* view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setModel( model );
        view->setFrameShape( QFrame::NoFrame );
        view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        model->append( artist );

        m_currentInterface = view->proxyModel();
        m_artistViews.insert( artist, view );

        m_stack->addWidget( view );
        m_stack->setCurrentWidget( view );
    }
    else
    {
        PlaylistView* view = m_artistViews.value( artist );
        m_stack->setCurrentWidget( view );
        m_currentInterface = view->proxyModel();
    }
    
    m_queueView->show();
    m_infobar->setCaption( tr( "All tracks by %1" ).arg( artist->name() ) );
    m_infobar->setDescription( "" );

    m_superCollectionVisible = false;
    m_statsAvailable = false;
    m_modesAvailable = false;
    linkPlaylist();

    emit numSourcesChanged( 1 );
    return true;
}


bool
PlaylistManager::show( const Tomahawk::album_ptr& album )
{
    qDebug() << Q_FUNC_INFO << &album << album.data();
    unlinkPlaylist();
    
    if ( !m_albumViews.contains( album ) )
    {
        PlaylistView* view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setModel( model );
        view->setFrameShape( QFrame::NoFrame );
        view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        model->append( album );

        m_currentInterface = view->proxyModel();
        m_albumViews.insert( album, view );

        m_stack->addWidget( view );
        m_stack->setCurrentWidget( view );
    }
    else
    {
        PlaylistView* view = m_albumViews.value( album );
        m_stack->setCurrentWidget( view );
        m_currentInterface = view->proxyModel();
    }
    
    m_queueView->show();
    m_infobar->setCaption( tr( "All tracks on %1 by %2" ).arg( album->name() ).arg( album->artist()->name() ) );
    m_infobar->setDescription( "" );

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
            view->setFrameShape( QFrame::NoFrame );
            view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
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
            aview->setFrameShape( QFrame::NoFrame );
            aview->setAttribute( Qt::WA_MacShowFocusRect, 0 );
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

    m_infobar->setDescription( "" );
    if ( collection->source()->isLocal()  )
        m_infobar->setCaption( tr( "Your Collection" ) );
    else
        m_infobar->setCaption( tr( "Collection of %1" ).arg( collection->source()->friendlyName() ) );
    
    m_queueView->show();
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

    m_infobar->setCaption( tr( "Info about %1" ).arg( source->isLocal() ? tr( "Your Collection" ) : source->friendlyName() ) );
    m_infobar->setDescription( "" );
    
    m_queueView->show();
    m_stack->setCurrentWidget( m_currentInfoWidget );
    m_superCollectionVisible = false;
    m_statsAvailable = false;
    m_modesAvailable = false;

    linkPlaylist();

    emit numSourcesChanged( 1 );
    return true;
}


bool
PlaylistManager::show( QWidget* widget, const QString& title, const QString& desc, const QPixmap& pixmap )
{
    unlinkPlaylist();

    if ( m_stack->indexOf( widget ) < 0 )
    {
        connect( widget, SIGNAL( destroyed( QWidget* ) ), SLOT( onWidgetDestroyed( QWidget* ) ) );
        m_stack->addWidget( widget );
    }

    m_stack->setCurrentWidget( widget );
    m_infobar->setCaption( title );
    m_infobar->setDescription( desc );
    m_infobar->setPixmap( pixmap.isNull() ? QPixmap( RESPATH "icons/tomahawk-icon-128x128.png" ) : pixmap );
    
    m_queueView->show();
    m_superCollectionVisible = false;
    m_statsAvailable = false;
    m_modesAvailable = false;
    m_currentInterface = 0;

    linkPlaylist();

    return true;
}


bool
PlaylistManager::showSuperCollection()
{
    foreach( const Tomahawk::source_ptr& source, SourceList::instance()->sources() )
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

    m_infobar->setCaption( tr( "Super Collection" ) );
    m_infobar->setDescription( tr( "All available tracks" ) );
    
    m_queueView->show();
    m_superCollectionVisible = true;
    m_statsAvailable = ( m_currentMode == 0 );
    m_modesAvailable = true;
    linkPlaylist();

    emit numSourcesChanged( m_superCollections.count() );
    return true;
}


void
PlaylistManager::showWelcomePage()
{
    qDebug() << Q_FUNC_INFO;
    show( m_welcomeWidget, tr( "Welcome to Tomahawk!" ) );
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

    if ( m_currentInterface && m_currentInterface->filter() != m_filter )
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

        m_interfaceHistory.removeAll( m_currentInterface );
        m_interfaceHistory << m_currentInterface;
    }

    AudioEngine::instance()->setPlaylist( m_currentInterface );

    if ( m_currentInterface && m_statsAvailable )
    {
        m_topbar->setFilter( m_currentInterface->filter() );

        emit numTracksChanged( m_currentInterface->unfilteredTrackCount() );
        emit numShownChanged( m_currentInterface->trackCount() );
        emit repeatModeChanged( m_currentInterface->repeatMode() );
        emit shuffleModeChanged( m_currentInterface->shuffled() );

    }

    emit statsAvailable( m_statsAvailable );
    emit modesAvailable( m_modesAvailable );
}


void
PlaylistManager::onWidgetDestroyed( QWidget* widget )
{
    qDebug() << "Destroyed child:" << widget;

    bool resetWidget = ( m_stack->currentWidget() == widget );
    m_stack->removeWidget( widget );

    if ( resetWidget && m_interfaceHistory.count() )
    {
        unlinkPlaylist();

        m_currentInterface = m_interfaceHistory.last();
        qDebug() << "Last interface:" << m_currentInterface << m_currentInterface->widget();

        if ( m_currentInterface->widget() )
            m_stack->setCurrentWidget( m_currentInterface->widget() );

        linkPlaylist();
    }
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
PlaylistManager::createPlaylist( const Tomahawk::source_ptr& src,
                                 const QVariant& contents )
{
    Tomahawk::playlist_ptr p = Tomahawk::playlist_ptr( new Tomahawk::Playlist( src ) );
    QJson::QObjectHelper::qvariant2qobject( contents.toMap(), p.data() );
    p->reportCreated( p );
}


void 
PlaylistManager::createDynamicPlaylist( const Tomahawk::source_ptr& src,
                                        const QVariant& contents )
{
    Tomahawk::dynplaylist_ptr p = Tomahawk::dynplaylist_ptr( new Tomahawk::DynamicPlaylist( src, contents.toMap().value( "type", QString() ).toString()  ) );
    QJson::QObjectHelper::qvariant2qobject( contents.toMap(), p.data() );
    p->reportCreated( p );
}


void
PlaylistManager::showCurrentTrack()
{
    unlinkPlaylist();

    m_currentInterface = AudioEngine::instance()->currentTrackPlaylist();

    if ( m_currentInterface->widget() )
        m_stack->setCurrentWidget( m_currentInterface->widget() );

    linkPlaylist();

/*    if ( m_currentView && m_currentProxyModel )
        m_currentView->scrollTo( m_currentProxyModel->currentItem(), QAbstractItemView::PositionAtCenter );*/
}


void
PlaylistManager::onPlayClicked()
{
    emit playClicked();
}


void
PlaylistManager::onPauseClicked()
{
    emit pauseClicked();
}