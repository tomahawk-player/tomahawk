/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2010-2012, Leo Franchi   <lfranchi@kde.org>
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

#include "audio/AudioEngine.h"
#include "context/ContextWidget.h"
#include "infobar/InfoBar.h"

#include "playlist/FlexibleView.h"
#include "playlist/TreeModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistView.h"
#include "playlist/PlayableProxyModel.h"
#include "playlist/PlayableModel.h"
#include "playlist/TreeView.h"
#include "playlist/GridView.h"
#include "playlist/AlbumModel.h"
#include "SourceList.h"
#include "TomahawkSettings.h"

#include "playlist/PlaylistLargeItemDelegate.h"
#include "playlist/RecentlyPlayedModel.h"
#include "playlist/dynamic/widgets/DynamicWidget.h"

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

#include <QVBoxLayout>
#include <QMetaMethod>


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
    , m_recentPlaysWidget( 0 )
    , m_currentPage( 0 )
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
//    m_superCollectionView->proxyModel()->setShowOfflineResults( false );

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
    delete m_whatsHotWidget;
    delete m_newReleasesWidget;
    delete m_welcomeWidget;
    delete m_recentPlaysWidget;
    delete m_contextWidget;
    delete m_widget;
}


FlexibleView*
ViewManager::createPageForPlaylist( const playlist_ptr& playlist )
{
    FlexibleView* view = new FlexibleView();
    PlaylistModel* model = new PlaylistModel();

    PlaylistView* pv = new PlaylistView();
    view->setDetailedView( pv );
    view->setPixmap( pv->pixmap() );
    view->setEmptyTip( tr( "This playlist is empty!" ) );

    // We need to set the model on the view before loading the playlist, so spinners & co are connected
    view->setPlaylistModel( model );
    pv->setPlaylistModel( model );

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
    m_currentCollection = collection;

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

    setPage( view );
    return view;
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

    setPage( m_superCollectionView );
    return m_superCollectionView;
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
ViewManager::showRecentPlaysPage()
{
    if ( !m_recentPlaysWidget )
    {
        FlexibleView* pv = new FlexibleView( m_widget );
        pv->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RecentlyPlayed ) );

        RecentlyPlayedModel* raModel = new RecentlyPlayedModel( pv );
        raModel->setTitle( tr( "Recently Played Tracks" ) );
        raModel->setDescription( tr( "Recently played tracks from all your friends" ) );

        PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::RecentlyPlayed, pv->trackView(), pv->trackView()->proxyModel() );
        connect( del, SIGNAL( updateIndex( QModelIndex ) ), pv->trackView(), SLOT( update( QModelIndex ) ) );
        pv->trackView()->setItemDelegate( del );

        pv->setPlayableModel( raModel );
        pv->setEmptyTip( tr( "Sorry, we could not find any recent plays!" ) );
        raModel->setSource( source_ptr() );

        pv->setGuid( "recentlyplayed" );

        m_recentPlaysWidget = pv;
    }

    return show( m_recentPlaysWidget );
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
    if ( m_currentPage )
        m_currentPage->setFilter( m_filter );
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
ViewManager::allPages() const
{
    QList< ViewPage* > pages = m_pageHistoryBack + m_pageHistoryFwd;
    pages << m_currentPage;

    return pages;
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
ViewManager::updateView()
{
    if ( currentPlaylistInterface() )
    {
        m_infobar->setFilter( currentPage()->filter() );
    }

/*    if ( currentPage()->queueVisible() )
        showQueue();
    else
        hideQueue();*/

    emit filterAvailable( currentPage()->showFilter() );

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
    QList< Tomahawk::ViewPage* > pages = allPages();

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


bool
ViewManager::isSuperCollectionVisible() const
{
    return ( currentPage() != 0 &&
           ( currentPage()->playlistInterface() == m_superCollectionView->playlistInterface() ) );
}


void
ViewManager::showCurrentTrack()
{
    ViewPage* page = pageForInterface( AudioEngine::instance()->currentTrackPlaylist() );

    if ( page )
    {
        setPage( page );
        page->jumpToCurrentTrack();
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
ViewManager::recentPlaysWidget() const
{
    return m_recentPlaysWidget;
}


TreeView*
ViewManager::superCollectionView() const
{
    return m_superCollectionView;
}
