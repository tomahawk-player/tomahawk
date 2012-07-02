/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ViewManager.h"

#include <QVBoxLayout>
#include <QMetaMethod>

#include "audio/AudioEngine.h"
#include "context/ContextWidget.h"
#include "infobar/InfoBar.h"
#include "topbar/TopBar.h"

#include "FlexibleView.h"
#include "TreeModel.h"
#include "PlaylistModel.h"
#include "PlaylistView.h"
#include "PlayableProxyModel.h"
#include "PlayableModel.h"
#include "TreeView.h"
#include "GridView.h"
#include "AlbumModel.h"
#include "SourceList.h"
#include "TomahawkSettings.h"

#include "CustomPlaylistView.h"
#include "PlaylistLargeItemDelegate.h"
#include "RecentlyPlayedModel.h"
#include "dynamic/widgets/DynamicWidget.h"

#include "widgets/NewReleasesWidget.h"
#include "widgets/WelcomeWidget.h"
#include "widgets/WhatsHotWidget.h"
#include "widgets/infowidgets/SourceInfoWidget.h"
#include "widgets/infowidgets/ArtistInfoWidget.h"
#include "widgets/infowidgets/AlbumInfoWidget.h"
#include "widgets/infowidgets/TrackInfoWidget.h"
#include "widgets/NewPlaylistWidget.h"
#include "widgets/AnimatedSplitter.h"

#include "utils/Logger.h"

#define FILTER_TIMEOUT 280

using namespace Tomahawk;

ViewManager* ViewManager::s_instance = 0;


ViewManager*
ViewManager::instance()
{
    return s_instance;
}


ViewManager::ViewManager( QObject* parent )
    : QObject( parent )
    , m_widget( new QWidget() )
    , m_welcomeWidget( new WelcomeWidget() )
    , m_whatsHotWidget( new WhatsHotWidget() )
    , m_newReleasesWidget( new NewReleasesWidget() )
    , m_topLovedWidget( 0 )
    , m_recentPlaysWidget( 0 )
    , m_currentPage( 0 )
    , m_currentMode( PlaylistModes::Tree )
    , m_loaded( false )
{
    s_instance = this;

    m_widget->setLayout( new QVBoxLayout() );
    m_infobar = new InfoBar();
    m_stack = new QStackedWidget();

    m_contextWidget = new ContextWidget();

    m_widget->layout()->addWidget( m_infobar );
    m_widget->layout()->addWidget( m_stack );
    m_widget->layout()->addWidget( m_contextWidget );

    m_superCollectionView = new TreeView();
    m_superCollectionView->proxyModel()->setStyle( PlayableProxyModel::Collection );
    m_superCollectionModel = new TreeModel( m_superCollectionView );
    m_superCollectionView->setTreeModel( m_superCollectionModel );
    m_superCollectionView->setShowModes( false );
//    m_superCollectionView->proxyModel()->setShowOfflineResults( false );

    m_superGridView = new GridView();
    m_superAlbumModel = new AlbumModel( m_superGridView );
    m_superGridView->setPlayableModel( m_superAlbumModel );

    m_stack->setContentsMargins( 0, 0, 0, 0 );
    m_widget->setContentsMargins( 0, 0, 0, 0 );
    m_widget->layout()->setContentsMargins( 0, 0, 0, 0 );
    m_widget->layout()->setMargin( 0 );
    m_widget->layout()->setSpacing( 0 );

    connect( AudioEngine::instance(), SIGNAL( playlistChanged( Tomahawk::playlistinterface_ptr ) ), this, SLOT( playlistInterfaceChanged( Tomahawk::playlistinterface_ptr ) ) );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );
    connect( m_infobar, SIGNAL( filterTextChanged( QString ) ), SLOT( setFilter( QString ) ) );

    connect( this, SIGNAL( tomahawkLoaded() ), m_whatsHotWidget, SLOT( fetchData() ) );
    connect( this, SIGNAL( tomahawkLoaded() ), m_newReleasesWidget, SLOT( fetchData() ) );
    connect( this, SIGNAL( tomahawkLoaded() ), m_welcomeWidget, SLOT( loadData() ) );

/*    connect( m_infobar, SIGNAL( flatMode() ), SLOT( setTableMode() ) );
    connect( m_infobar, SIGNAL( artistMode() ), SLOT( setTreeMode() ) );
    connect( m_infobar, SIGNAL( albumMode() ), SLOT( setAlbumMode() ) );*/
}


ViewManager::~ViewManager()
{
    saveCurrentPlaylistSettings();
    delete m_whatsHotWidget;
    delete m_newReleasesWidget;
    delete m_welcomeWidget;
    delete m_topLovedWidget;
    delete m_recentPlaysWidget;
    delete m_contextWidget;
    delete m_widget;
}


FlexibleView*
ViewManager::createPageForPlaylist( const playlist_ptr& playlist )
{
    FlexibleView* view = new FlexibleView();
    PlaylistModel* model = new PlaylistModel();
    view->setPlayableModel( model );

    PlaylistView* pv = new PlaylistView();
    pv->setPlaylistModel( model );
    view->setDetailedView( pv );

    model->loadPlaylist( playlist );
    playlist->resolve();

    return view;
}


playlist_ptr
ViewManager::playlistForPage( ViewPage* page ) const
{
    playlist_ptr p;
    if ( dynamic_cast< PlaylistView* >( page ) && dynamic_cast< PlaylistView* >( page )->playlistModel() &&
        !dynamic_cast< PlaylistView* >( page )->playlistModel()->playlist().isNull() )
    {
        p = dynamic_cast< PlaylistView* >( page )->playlistModel()->playlist();
    }
    else if ( dynamic_cast< DynamicWidget* >( page ) )
        p = dynamic_cast< DynamicWidget* >( page )->playlist();

    return p;
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::playlist_ptr& playlist )
{
    FlexibleView* view;

    if ( !m_playlistViews.contains( playlist ) || m_playlistViews.value( playlist ).isNull() )
    {
        view = createPageForPlaylist( playlist );
        m_playlistViews.insert( playlist, view );
    }
    else
    {
        view = m_playlistViews.value( playlist ).data();
    }

    setPage( view );
    emit numSourcesChanged( SourceList::instance()->count() );

    return view;
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::dynplaylist_ptr& playlist )
{
    if ( !m_dynamicWidgets.contains( playlist ) || m_dynamicWidgets.value( playlist ).isNull() )
    {
       m_dynamicWidgets[ playlist ] = new Tomahawk::DynamicWidget( playlist, m_stack );

       playlist->resolve();
    }

    setPage( m_dynamicWidgets.value( playlist ).data() );

/*    if ( playlist->mode() == Tomahawk::OnDemand )
        hideQueue();
    else
        showQueue();*/

    emit numSourcesChanged( SourceList::instance()->count() );

    return m_dynamicWidgets.value( playlist ).data();
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::artist_ptr& artist )
{
    ArtistInfoWidget* swidget;
    if ( !m_artistViews.contains( artist ) || m_artistViews.value( artist ).isNull() )
    {
        swidget = new ArtistInfoWidget( artist );
        m_artistViews.insert( artist, swidget );
    }
    else
    {
        swidget = m_artistViews.value( artist ).data();
    }

    setPage( swidget );
    return swidget;
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::album_ptr& album )
{
    AlbumInfoWidget* swidget;
    if ( !m_albumViews.contains( album ) || m_albumViews.value( album ).isNull() )
    {
        swidget = new AlbumInfoWidget( album );
        m_albumViews.insert( album, swidget );
    }
    else
    {
        swidget = m_albumViews.value( album ).data();
    }

    setPage( swidget );
    return swidget;
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::query_ptr& query )
{
    TrackInfoWidget* swidget;
    if ( !m_trackViews.contains( query ) || m_trackViews.value( query ).isNull() )
    {
        swidget = new TrackInfoWidget( query );
        m_trackViews.insert( query, swidget );
    }
    else
    {
        swidget = m_trackViews.value( query ).data();
    }

    setPage( swidget );
    return swidget;
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO << m_currentMode;
    m_currentCollection = collection;
    ViewPage* shown = 0;
    if ( m_currentMode == PlaylistModes::Flat )
    {
/*        CollectionView* view;
        if ( !m_collectionViews.contains( collection ) || m_collectionViews.value( collection ).isNull() )
        {
            view = new CollectionView();
            CollectionFlatModel* model = new CollectionFlatModel();
            view->setPlayableModel( model );

            model->addCollection( collection );

            m_collectionViews.insert( collection, view );
        }
        else
        {
            view = m_collectionViews.value( collection ).data();
        }

        shown = view;
        setPage( view );*/
    }

    if ( m_currentMode == PlaylistModes::Tree )
    {
        TreeView* view;
        if ( !m_treeViews.contains( collection ) || m_treeViews.value( collection ).isNull() )
        {
            view = new TreeView();
            view->proxyModel()->setStyle( PlayableProxyModel::Collection );
            TreeModel* model = new TreeModel();
            view->setTreeModel( model );

            if ( collection && collection->source()->isLocal() )
                view->setEmptyTip( tr( "After you have scanned your music collection you will find your tracks right here." ) );
            else
                view->setEmptyTip( tr( "This collection is empty." ) );

            model->addCollection( collection );

            m_treeViews.insert( collection, view );
        }
        else
        {
            view = m_treeViews.value( collection ).data();
        }

        shown = view;
        setPage( view );
    }

    if ( m_currentMode == PlaylistModes::Album )
    {
        GridView* aview;
        if ( !m_collectionGridViews.contains( collection ) || m_collectionGridViews.value( collection ).isNull() )
        {
            aview = new GridView();
            AlbumModel* amodel = new AlbumModel( aview );
            aview->setPlayableModel( amodel );
            amodel->addCollection( collection );

            m_collectionGridViews.insert( collection, aview );
        }
        else
        {
            aview = m_collectionGridViews.value( collection ).data();
        }

        shown = aview;
        setPage( aview );
    }

    emit numSourcesChanged( 1 );

    return shown;
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::source_ptr& source )
{
    SourceInfoWidget* swidget;
    if ( !m_sourceViews.contains( source ) || m_sourceViews.value( source ).isNull() )
    {
        swidget = new SourceInfoWidget( source );
        m_sourceViews.insert( source, swidget );
    }
    else
    {
        swidget = m_sourceViews.value( source ).data();
    }

    setPage( swidget );
    return swidget;
}


Tomahawk::ViewPage*
ViewManager::show( ViewPage* page )
{
    setPage( page );

    return page;
}


Tomahawk::ViewPage*
ViewManager::showSuperCollection()
{
    if ( m_superCollections.isEmpty() )
        m_superCollectionModel->addAllCollections();

    foreach( const Tomahawk::source_ptr& source, SourceList::instance()->sources() )
    {
        if ( !m_superCollections.contains( source->collection() ) )
        {
            m_superCollections.append( source->collection() );
//            m_superAlbumModel->addCollection( source->collection() );
        }
    }

    m_superCollectionModel->setTitle( tr( "SuperCollection" ) );
    m_superCollectionModel->setDescription( tr( "Combined libraries of all your online friends" ) );
    m_superAlbumModel->setTitle( tr( "All available albums" ) );

    ViewPage* shown = 0;
    if ( m_currentMode == PlaylistModes::Tree )
    {
        shown = m_superCollectionView;
        setPage( m_superCollectionView );
    }
    else if ( m_currentMode == PlaylistModes::Flat )
    {
        shown = m_superCollectionView;
        setPage( m_superCollectionView );
    }
    else if ( m_currentMode == PlaylistModes::Album )
    {
        shown = m_superGridView;
        setPage( m_superGridView );
    }

    emit numSourcesChanged( m_superCollections.count() );

    return shown;
}


void
ViewManager::playlistInterfaceChanged( Tomahawk::playlistinterface_ptr interface )
{
    playlist_ptr pl = playlistForInterface( interface );
    if ( !pl.isNull() )
    {
        TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( pl->guid(), pl->author()->id() );
    }
    else
    {
        pl = dynamicPlaylistForInterface( interface );
        if ( !pl.isNull() )
            TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( pl->guid(), pl->author()->id() );
    }
}


Tomahawk::ViewPage*
ViewManager::showWelcomePage()
{
    return show( m_welcomeWidget );
}


Tomahawk::ViewPage*
ViewManager::showWhatsHotPage()
{
    return show( m_whatsHotWidget );
}


Tomahawk::ViewPage*
ViewManager::showNewReleasesPage()
{
    return show( m_newReleasesWidget );
}


Tomahawk::ViewPage*
ViewManager::showTopLovedPage()
{
    if ( !m_topLovedWidget )
    {
        CustomPlaylistView* view = new CustomPlaylistView( CustomPlaylistView::TopLovedTracks, source_ptr(), m_widget );
        PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::LovedTracks, view, view->proxyModel() );
        connect( del, SIGNAL( updateIndex( QModelIndex ) ), view, SLOT( update( QModelIndex ) ) );
        view->setItemDelegate( del );

        m_topLovedWidget = view;
    }

    return show( m_topLovedWidget );
}


Tomahawk::ViewPage*
ViewManager::showRecentPlaysPage()
{
    if ( !m_recentPlaysWidget )
    {
        PlaylistView* pv = new PlaylistView( m_widget );

        RecentlyPlayedModel* raModel = new RecentlyPlayedModel( pv );
        raModel->setTitle( tr( "Recently Played Tracks" ) );
        raModel->setDescription( tr( "Recently played tracks from all your friends" ) );
        pv->proxyModel()->setStyle( PlayableProxyModel::Large );

        PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::RecentlyPlayed, pv, pv->proxyModel() );
        connect( del, SIGNAL( updateIndex( QModelIndex ) ), pv, SLOT( update( QModelIndex ) ) );
        pv->setItemDelegate( del );

        pv->setPlaylistModel( raModel );
        raModel->setSource( source_ptr() );

        m_recentPlaysWidget = pv;
    }

    return show( m_recentPlaysWidget );
}


void
ViewManager::setTableMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = PlaylistModes::Flat;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
ViewManager::setTreeMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = PlaylistModes::Tree;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
ViewManager::setAlbumMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = PlaylistModes::Album;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
ViewManager::setFilter( const QString& filter )
{
    m_filter = filter;

    m_filterTimer.stop();
    m_filterTimer.setInterval( FILTER_TIMEOUT );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}


void
ViewManager::applyFilter()
{
    if ( currentPlaylistInterface() && currentPlaylistInterface()->filter() != m_filter )
        currentPlaylistInterface()->setFilter( m_filter );
}


void
ViewManager::historyBack()
{
    if ( !m_pageHistoryBack.count() )
        return;

    ViewPage* page = m_pageHistoryBack.takeLast();

    if ( m_currentPage )
    {
        m_pageHistoryFwd << m_currentPage;
        tDebug() << "Moved to forward history:" << m_currentPage->widget()->metaObject()->className();
    }

    tDebug() << "Showing page after moving backwards in history:" << page->widget()->metaObject()->className();
    setPage( page, false );
}


void
ViewManager::historyForward()
{
    if ( !m_pageHistoryFwd.count() )
        return;

    ViewPage* page = m_pageHistoryFwd.takeLast();

    if ( m_currentPage )
    {
        m_pageHistoryBack << m_currentPage;
        tDebug() << "Moved to backward history:" << m_currentPage->widget()->metaObject()->className();
    }

    tDebug() << "Showing page after moving forwards in history:" << page->widget()->metaObject()->className();
    setPage( page, false );
}


QList<ViewPage*>
ViewManager::historyPages() const
{
    return m_pageHistoryBack + m_pageHistoryFwd;
}


void
ViewManager::destroyPage( ViewPage* page )
{
    if ( !page )
        return;

    tDebug() << Q_FUNC_INFO << "Deleting page:" << page->title();
    if ( historyPages().contains( page ) )
    {
        m_pageHistoryBack.removeAll( page );
        m_pageHistoryFwd.removeAll( page );

        emit historyBackAvailable( m_pageHistoryBack.count() );
        emit historyForwardAvailable( m_pageHistoryFwd.count() );

        delete page;
    }

    if ( m_currentPage == page )
    {
        m_currentPage = 0;

        historyBack();
    }
}


void
ViewManager::setPage( ViewPage* page, bool trackHistory )
{
    if ( !page )
        return;
    if ( page == m_currentPage )
        return;

    // save the old playlist shuffle state in config before we change playlists
    saveCurrentPlaylistSettings();
    unlinkPlaylist();

    if ( m_stack->indexOf( page->widget() ) < 0 )
    {
        m_stack->addWidget( page->widget() );
    }

    if ( m_currentPage && trackHistory )
    {
        m_pageHistoryBack << m_currentPage;
        m_pageHistoryFwd.clear();
    }
    m_currentPage = page;

    emit historyBackAvailable( m_pageHistoryBack.count() );
    emit historyForwardAvailable( m_pageHistoryFwd.count() );

    qDebug() << "View page shown:" << page->title();
    emit viewPageActivated( page );

    if ( page->isTemporaryPage() )
        emit tempPageActivated( page );

    if ( AudioEngine::instance()->state() == AudioEngine::Stopped )
        AudioEngine::instance()->setPlaylist( page->playlistInterface() );

    // UGH!
    if ( QObject* obj = dynamic_cast< QObject* >( currentPage() ) )
    {
        // if the signal exists (just to hide the qobject runtime warning...)
        if( obj->metaObject()->indexOfSignal( "descriptionChanged(QString)" ) > -1 )
            connect( obj, SIGNAL( descriptionChanged( QString ) ), m_infobar, SLOT( setDescription( QString ) ), Qt::UniqueConnection );

        if( obj->metaObject()->indexOfSignal( "descriptionChanged(Tomahawk::artist_ptr)" ) > -1 )
            connect( obj, SIGNAL( descriptionChanged( Tomahawk::artist_ptr ) ), m_infobar, SLOT( setDescription( Tomahawk::artist_ptr ) ), Qt::UniqueConnection );

        if( obj->metaObject()->indexOfSignal( "descriptionChanged(Tomahawk::album_ptr)" ) > -1 )
            connect( obj, SIGNAL( descriptionChanged( Tomahawk::album_ptr ) ), m_infobar, SLOT( setDescription( Tomahawk::album_ptr ) ), Qt::UniqueConnection );

        if( obj->metaObject()->indexOfSignal( "longDescriptionChanged(QString)" ) > -1 )
            connect( obj, SIGNAL( longDescriptionChanged( QString ) ), m_infobar, SLOT( setLongDescription( QString ) ), Qt::UniqueConnection );

        if( obj->metaObject()->indexOfSignal( "nameChanged(QString)" ) > -1 )
            connect( obj, SIGNAL( nameChanged( QString ) ), m_infobar, SLOT( setCaption( QString ) ), Qt::UniqueConnection );

        if( obj->metaObject()->indexOfSignal( "pixmapChanged(QPixmap)" ) > -1 )
            connect( obj, SIGNAL( pixmapChanged( QPixmap ) ), m_infobar, SLOT( setPixmap( QPixmap ) ), Qt::UniqueConnection );

        if( obj->metaObject()->indexOfSignal( "destroyed(QWidget*)" ) > -1 )
            connect( obj, SIGNAL( destroyed( QWidget* ) ), SLOT( onWidgetDestroyed( QWidget* ) ), Qt::UniqueConnection );
    }

    m_stack->setCurrentWidget( page->widget() );

    updateView();
}


bool
ViewManager::isNewPlaylistPageVisible() const
{
    return dynamic_cast< NewPlaylistWidget* >( currentPage() ) != 0;
}


void
ViewManager::unlinkPlaylist()
{
    if ( currentPlaylistInterface() )
    {
        disconnect( currentPlaylistInterface().data(), SIGNAL( sourceTrackCountChanged( unsigned int ) ),
                    this,                                 SIGNAL( numTracksChanged( unsigned int ) ) );

        disconnect( currentPlaylistInterface().data(), SIGNAL( trackCountChanged( unsigned int ) ),
                    this,                                 SIGNAL( numShownChanged( unsigned int ) ) );

        disconnect( currentPlaylistInterface().data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ),
                    this,                                 SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );

        disconnect( currentPlaylistInterface().data(), SIGNAL( shuffleModeChanged( bool ) ),
                    this,                                 SIGNAL( shuffleModeChanged( bool ) ) );
    }
}


void
ViewManager::saveCurrentPlaylistSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    Tomahawk::playlist_ptr pl = playlistForInterface( currentPlaylistInterface() );

    if ( !pl.isNull() )
    {
        s->setShuffleState(  pl->guid(), currentPlaylistInterface()->shuffled() );
        s->setRepeatMode( pl->guid(), currentPlaylistInterface()->repeatMode() );
    }
    else
    {
        Tomahawk::dynplaylist_ptr dynPl = dynamicPlaylistForInterface( currentPlaylistInterface() );
        if ( !dynPl.isNull() )
        {
            s->setShuffleState( dynPl->guid(), currentPlaylistInterface()->shuffled() );
            s->setRepeatMode( dynPl->guid(), currentPlaylistInterface()->repeatMode() );
        }
    }
}


void
ViewManager::updateView()
{
    if ( currentPlaylistInterface() )
    {
        connect( currentPlaylistInterface().data(), SIGNAL( sourceTrackCountChanged( unsigned int ) ),
                                                    SIGNAL( numTracksChanged( unsigned int ) ) );

        connect( currentPlaylistInterface().data(), SIGNAL( trackCountChanged( unsigned int ) ),
                                                    SIGNAL( numShownChanged( unsigned int ) ) );

        connect( currentPlaylistInterface().data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ),
                                                    SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );

        connect( currentPlaylistInterface().data(), SIGNAL( shuffleModeChanged( bool ) ),
                                                    SIGNAL( shuffleModeChanged( bool ) ) );

        m_infobar->setFilter( currentPlaylistInterface()->filter() );
    }

    if ( currentPage()->showStatsBar() && currentPlaylistInterface() )
    {
        emit numTracksChanged( currentPlaylistInterface()->unfilteredTrackCount() );

        if ( !currentPlaylistInterface()->filter().isEmpty() )
            emit numShownChanged( currentPlaylistInterface()->trackCount() );
        else
            emit numShownChanged( currentPlaylistInterface()->unfilteredTrackCount() );

        emit repeatModeChanged( currentPlaylistInterface()->repeatMode() );
        emit shuffleModeChanged( currentPlaylistInterface()->shuffled() );
        emit modeChanged( currentPlaylistInterface()->viewMode() );
    }

/*    if ( currentPage()->queueVisible() )
        showQueue();
    else
        hideQueue();*/

    emit statsAvailable( currentPage()->showStatsBar() );
    emit modesAvailable( currentPage()->showModes() );
    emit filterAvailable( currentPage()->showFilter() );

/*    if ( !currentPage()->showStatsBar() && !currentPage()->showModes() && !currentPage()->showFilter() )
        m_topbar->setVisible( false );
    else
        m_topbar->setVisible( true );*/

    m_infobar->setVisible( currentPage()->showInfoBar() );
    m_infobar->setCaption( currentPage()->title() );

    m_infobar->setUpdaters( currentPage()->updaters() );

    switch( currentPage()->descriptionType() )
    {
        case ViewPage::TextType:
            m_infobar->setDescription( currentPage()->description() );
            break;
        case ViewPage::ArtistType:
            m_infobar->setDescription( currentPage()->descriptionArtist() );
            break;
        case ViewPage::AlbumType:
            m_infobar->setDescription( currentPage()->descriptionAlbum() );
            break;

    }
    m_infobar->setLongDescription( currentPage()->longDescription() );
    m_infobar->setPixmap( currentPage()->pixmap() );

    // turn on shuffle/repeat mode for the new playlist view if specified in config
    loadCurrentPlaylistSettings();
}


void
ViewManager::loadCurrentPlaylistSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    Tomahawk::playlist_ptr pl = playlistForInterface( currentPlaylistInterface() );

    if ( !pl.isNull() )
    {
        currentPlaylistInterface()->setShuffled( s->shuffleState( pl->guid() ));
        currentPlaylistInterface()->setRepeatMode( s->repeatMode( pl->guid() ));
    }
    else
    {
        Tomahawk::dynplaylist_ptr dynPl = dynamicPlaylistForInterface( currentPlaylistInterface() );
        if ( !dynPl.isNull() )
        {
            currentPlaylistInterface()->setShuffled( s->shuffleState( dynPl->guid() ));
        }
    }
}


void
ViewManager::onWidgetDestroyed( QWidget* widget )
{
    tDebug() << "Destroyed child:" << widget << widget->metaObject()->className();

    bool resetWidget = ( m_stack->currentWidget() == widget );

    QList< Tomahawk::ViewPage* > p = historyPages();
    for ( int i = 0; i < p.count(); i++ )
    {
        ViewPage* page = p.at( i );
        if ( page->widget() != widget )
            continue;

        if ( !playlistForInterface( page->playlistInterface() ).isNull() )
        {
            m_playlistViews.remove( playlistForInterface( page->playlistInterface() ) );
        }
        if ( !dynamicPlaylistForInterface( page->playlistInterface() ).isNull() )
        {
            m_dynamicWidgets.remove( dynamicPlaylistForInterface( page->playlistInterface() ) );
        }

        m_pageHistoryBack.removeAll( page );
        m_pageHistoryFwd.removeAll( page );
    }

    m_stack->removeWidget( widget );

    if ( resetWidget )
    {
        m_currentPage = 0;
        historyBack();
    }
}


void
ViewManager::setRepeatMode( Tomahawk::PlaylistModes::RepeatMode mode )
{
    if ( currentPlaylistInterface() )
        currentPlaylistInterface()->setRepeatMode( mode );
}


void
ViewManager::setShuffled( bool enabled )
{
    if ( currentPlaylistInterface() )
        currentPlaylistInterface()->setShuffled( enabled );
}


void
ViewManager::createPlaylist( const Tomahawk::source_ptr& src, const QVariant& contents )
{
    Tomahawk::playlist_ptr p = Tomahawk::playlist_ptr( new Tomahawk::Playlist( src ) );
    QJson::QObjectHelper::qvariant2qobject( contents.toMap(), p.data() );
    p->reportCreated( p );
}


void
ViewManager::createDynamicPlaylist( const Tomahawk::source_ptr& src, const QVariant& contents )
{
    Tomahawk::dynplaylist_ptr p = Tomahawk::dynplaylist_ptr( new Tomahawk::DynamicPlaylist( src, contents.toMap().value( "type", QString() ).toString()  ) );
    QJson::QObjectHelper::qvariant2qobject( contents.toMap(), p.data() );
    p->reportCreated( p );
}


void
ViewManager::setTomahawkLoaded()
{
    m_loaded = true;
    emit tomahawkLoaded();
}


ViewPage*
ViewManager::pageForDynPlaylist(const dynplaylist_ptr& pl) const
{
    return m_dynamicWidgets.value( pl ).data();
}


ViewPage*
ViewManager::pageForPlaylist(const playlist_ptr& pl) const
{
    return m_playlistViews.value( pl ).data();
}


ViewPage*
ViewManager::pageForInterface( Tomahawk::playlistinterface_ptr interface ) const
{
    QList< Tomahawk::ViewPage* > pages = historyPages();

    for ( int i = 0; i < pages.count(); i++ )
    {
        ViewPage* page = pages.at( i );
        if ( page->playlistInterface() == interface )
            return page;
        if ( page->playlistInterface() && page->playlistInterface()->hasChildInterface( interface ) )
            return page;
    }

    return 0;
}


Tomahawk::playlistinterface_ptr
ViewManager::currentPlaylistInterface() const
{
    if ( currentPage() )
        return currentPage()->playlistInterface();
    else
        return Tomahawk::playlistinterface_ptr();
}


Tomahawk::ViewPage*
ViewManager::currentPage() const
{
    return m_currentPage;
}


Tomahawk::playlist_ptr
ViewManager::playlistForInterface( Tomahawk::playlistinterface_ptr interface ) const
{
    foreach ( QWeakPointer<FlexibleView> view, m_playlistViews.values() )
    {
        if ( !view.isNull() && view.data()->playlistInterface() == interface )
        {
            return m_playlistViews.key( view );
        }
    }

    return Tomahawk::playlist_ptr();
}


Tomahawk::dynplaylist_ptr
ViewManager::dynamicPlaylistForInterface( Tomahawk::playlistinterface_ptr interface ) const
{
    foreach ( QWeakPointer<DynamicWidget> view, m_dynamicWidgets.values() )
    {
        if ( !view.isNull() && view.data()->playlistInterface() == interface )
        {
            return m_dynamicWidgets.key( view );
        }
    }

    return Tomahawk::dynplaylist_ptr();
}


Tomahawk::collection_ptr
ViewManager::collectionForInterface( Tomahawk::playlistinterface_ptr interface ) const
{
    foreach ( QWeakPointer<GridView> view, m_collectionGridViews.values() )
    {
        if ( view.data()->playlistInterface() == interface )
        {
            return m_collectionGridViews.key( view );
        }
    }

    return collection_ptr();
}


bool
ViewManager::isSuperCollectionVisible() const
{
    return ( currentPage() != 0 &&
           ( currentPage()->playlistInterface() == m_superCollectionView->playlistInterface() ||
             currentPage()->playlistInterface() == m_superGridView->playlistInterface() ) );
}


void
ViewManager::showCurrentTrack()
{
    ViewPage* page = pageForInterface( AudioEngine::instance()->currentTrackPlaylist() );

    if ( page )
    {
        setPage( page );
        page->jumpToCurrentTrack();

        // reset the correct mode, if the user has changed it since

        if ( dynamic_cast< TrackView* >( page ) )
            m_currentMode = PlaylistModes::Flat;
        else if ( dynamic_cast< GridView* >( page ) )
            m_currentMode = PlaylistModes::Album;
        else if ( dynamic_cast< TreeView* >( page ) )
            m_currentMode = PlaylistModes::Tree;
        else
            return;

        emit modeChanged( (PlaylistModes::ViewMode)m_currentMode );
    }
}


Tomahawk::ViewPage*
ViewManager::welcomeWidget() const
{
    return m_welcomeWidget;
}


Tomahawk::ViewPage*
ViewManager::whatsHotWidget() const
{
    return m_whatsHotWidget;
}


Tomahawk::ViewPage*
ViewManager::newReleasesWidget() const
{
    return m_newReleasesWidget;
}


Tomahawk::ViewPage*
ViewManager::topLovedWidget() const
{
    return m_topLovedWidget;
}


Tomahawk::ViewPage*
ViewManager::recentPlaysWidget() const
{
    return m_recentPlaysWidget;
}


TreeView*
ViewManager::superCollectionView() const
{
    return m_superCollectionView;
}
