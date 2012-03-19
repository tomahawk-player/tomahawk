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

#include "viewmanager.h"

#include <QVBoxLayout>
#include <QMetaMethod>

#include "audio/audioengine.h"
#include "context/ContextWidget.h"
#include "infobar/infobar.h"
#include "topbar/topbar.h"

#include "treemodel.h"
#include "collectionflatmodel.h"
#include "collectionview.h"
#include "playlistmodel.h"
#include "playlistview.h"
#include "trackproxymodel.h"
#include "trackmodel.h"
#include "artistview.h"
#include "albumview.h"
#include "albumproxymodel.h"
#include "albummodel.h"
#include "sourcelist.h"
#include "tomahawksettings.h"

#include "customplaylistview.h"
#include "PlaylistLargeItemDelegate.h"
#include "dynamic/widgets/DynamicWidget.h"

#include "widgets/welcomewidget.h"
#include "widgets/whatshotwidget.h"
#include "widgets/infowidgets/sourceinfowidget.h"
#include "widgets/infowidgets/ArtistInfoWidget.h"
#include "widgets/infowidgets/AlbumInfoWidget.h"
#include "widgets/newplaylistwidget.h"
#include "widgets/animatedsplitter.h"

#include "utils/logger.h"

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
    , m_topLovedWidget( 0 )
    , m_currentMode( PlaylistInterface::Tree )
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

    m_superCollectionView = new ArtistView();
    m_superCollectionModel = new TreeModel( m_superCollectionView );
    m_superCollectionView->setTreeModel( m_superCollectionModel );
    m_superCollectionView->setFrameShape( QFrame::NoFrame );
    m_superCollectionView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    m_superCollectionView->setShowModes( false );
//    m_superCollectionView->proxyModel()->setShowOfflineResults( false );

    m_superAlbumView = new AlbumView();
    m_superAlbumModel = new AlbumModel( m_superAlbumView );
    m_superAlbumView->setAlbumModel( m_superAlbumModel );
    m_superAlbumView->setFrameShape( QFrame::NoFrame );
    m_superAlbumView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    m_stack->setContentsMargins( 0, 0, 0, 0 );
    m_widget->setContentsMargins( 0, 0, 0, 0 );
    m_widget->layout()->setContentsMargins( 0, 0, 0, 0 );
    m_widget->layout()->setMargin( 0 );
    m_widget->layout()->setSpacing( 0 );

    connect( AudioEngine::instance(), SIGNAL( playlistChanged( Tomahawk::playlistinterface_ptr ) ), this, SLOT( playlistInterfaceChanged( Tomahawk::playlistinterface_ptr ) ) );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );
    connect( m_infobar, SIGNAL( filterTextChanged( QString ) ), SLOT( setFilter( QString ) ) );
    connect( m_infobar, SIGNAL( autoUpdateChanged( int ) ), SLOT( autoUpdateChanged( int ) ) );

    connect( this, SIGNAL( tomahawkLoaded() ), m_whatsHotWidget, SLOT( fetchData() ) );
    connect( this, SIGNAL( tomahawkLoaded() ), m_welcomeWidget, SLOT( loadData() ) );

/*    connect( m_infobar, SIGNAL( flatMode() ), SLOT( setTableMode() ) );
    connect( m_infobar, SIGNAL( artistMode() ), SLOT( setTreeMode() ) );
    connect( m_infobar, SIGNAL( albumMode() ), SLOT( setAlbumMode() ) );*/
}


ViewManager::~ViewManager()
{
    saveCurrentPlaylistSettings();
    delete m_whatsHotWidget;
    delete m_welcomeWidget;
    delete m_topLovedWidget;
    delete m_contextWidget;
    delete m_widget;
}


PlaylistView*
ViewManager::createPageForPlaylist( const playlist_ptr& pl )
{
    PlaylistView* view = new PlaylistView();
    PlaylistModel* model = new PlaylistModel();
    view->setPlaylistModel( model );
    model->loadPlaylist( pl );
    view->setFrameShape( QFrame::NoFrame );
    view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    pl->resolve();

    m_playlistViews.insert( pl, view );
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
    PlaylistView* view;

    if ( !m_playlistViews.contains( playlist ) || m_playlistViews.value( playlist ).isNull() )
    {
        view = createPageForPlaylist( playlist );
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
ViewManager::show( const Tomahawk::album_ptr& album, Tomahawk::ModelMode initialMode )
{
    AlbumInfoWidget* swidget;
    if ( !m_albumViews.contains( album ) || m_albumViews.value( album ).isNull() )
    {
        swidget = new AlbumInfoWidget( album, initialMode );
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
ViewManager::show( const Tomahawk::collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO << m_currentMode;
    m_currentCollection = collection;
    ViewPage* shown = 0;
    if ( m_currentMode == PlaylistInterface::Flat )
    {
        CollectionView* view;
        if ( !m_collectionViews.contains( collection ) || m_collectionViews.value( collection ).isNull() )
        {
            view = new CollectionView();
            CollectionFlatModel* model = new CollectionFlatModel();
            view->setTrackModel( model );
            view->setFrameShape( QFrame::NoFrame );
            view->setAttribute( Qt::WA_MacShowFocusRect, 0 );

            model->addCollection( collection );

            m_collectionViews.insert( collection, view );
        }
        else
        {
            view = m_collectionViews.value( collection ).data();
        }

        shown = view;
        setPage( view );
    }

    if ( m_currentMode == PlaylistInterface::Tree )
    {
        ArtistView* view;
        if ( !m_treeViews.contains( collection ) || m_treeViews.value( collection ).isNull() )
        {
            view = new ArtistView();
            TreeModel* model = new TreeModel();
            view->setTreeModel( model );
            view->setFrameShape( QFrame::NoFrame );
            view->setAttribute( Qt::WA_MacShowFocusRect, 0 );

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

    if ( m_currentMode == PlaylistInterface::Album )
    {
        AlbumView* aview;
        if ( !m_collectionAlbumViews.contains( collection ) || m_collectionAlbumViews.value( collection ).isNull() )
        {
            aview = new AlbumView();
            AlbumModel* amodel = new AlbumModel( aview );
            aview->setAlbumModel( amodel );
            aview->setFrameShape( QFrame::NoFrame );
            aview->setAttribute( Qt::WA_MacShowFocusRect, 0 );
            amodel->addCollection( collection );

            m_collectionAlbumViews.insert( collection, aview );
        }
        else
        {
            aview = m_collectionAlbumViews.value( collection ).data();
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
    if ( m_currentMode == PlaylistInterface::Tree )
    {
        shown = m_superCollectionView;
        setPage( m_superCollectionView );
    }
    else if ( m_currentMode == PlaylistInterface::Flat )
    {
        shown = m_superCollectionView;
        setPage( m_superCollectionView );
    }
    else if ( m_currentMode == PlaylistInterface::Album )
    {
        shown = m_superAlbumView;
        setPage( m_superAlbumView );
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
        TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( pl );
    }
    else
    {
        pl = dynamicPlaylistForInterface( interface );
        if ( !pl.isNull() )
            TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( pl );
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
ViewManager::showTopLovedPage()
{
    if ( !m_topLovedWidget )
    {
        CustomPlaylistView* view = new CustomPlaylistView( CustomPlaylistView::TopLovedTracks, source_ptr(), m_widget );
        view->setItemDelegate( new PlaylistLargeItemDelegate( view, view->proxyModel() ) );

        m_topLovedWidget = view;
    }

    return show( m_topLovedWidget );
}


void
ViewManager::setTableMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = PlaylistInterface::Flat;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
ViewManager::setTreeMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = PlaylistInterface::Tree;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
ViewManager::setAlbumMode()
{
    qDebug() << Q_FUNC_INFO;

    m_currentMode = PlaylistInterface::Album;

    if ( isSuperCollectionVisible() )
        showSuperCollection();
    else
        show( m_currentCollection );
}


void
ViewManager::historyBack()
{
    ViewPage* oldPage = m_pageHistory.takeFirst();

    ViewPage* newPage = m_pageHistory.first();
    qDebug() << "Showing page after moving backwards in history:" << newPage->widget()->metaObject()->className();
    setPage( newPage, false );

    delete oldPage;
}


void
ViewManager::removeFromHistory ( ViewPage* p )
{
    if ( currentPage() == p )
    {
        historyBack();
    }
    else
    {
        m_pageHistory.removeAll( p );
        delete p;
    }

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
    qDebug() << Q_FUNC_INFO;

    if ( currentPlaylistInterface() && currentPlaylistInterface()->filter() != m_filter )
        currentPlaylistInterface()->setFilter( m_filter );
}


void
ViewManager::autoUpdateChanged( int state )
{
    currentPage()->setAutoUpdate( state == Qt::Checked );
}


void
ViewManager::setPage( ViewPage* page, bool trackHistory )
{
    if ( !page )
        return;

    // save the old playlist shuffle state in config before we change playlists
    saveCurrentPlaylistSettings();
    unlinkPlaylist();

    if ( !m_pageHistory.contains( page ) )
    {
        m_stack->addWidget( page->widget() );
    }
    else
    {
        if ( trackHistory )
            m_pageHistory.removeAll( page );
    }

    if ( trackHistory )
    {
        m_pageHistory.insert( 0, page );
    }

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

        disconnect( currentPlaylistInterface().data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ),
                    this,                                 SIGNAL( repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ) );

        disconnect( currentPlaylistInterface().data(), SIGNAL( shuffleModeChanged( bool ) ),
                    this,                                 SIGNAL( shuffleModeChanged( bool ) ) );
    }
}


void
ViewManager::saveCurrentPlaylistSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    Tomahawk::playlist_ptr pl = playlistForInterface( currentPlaylistInterface() );

    if ( !pl.isNull() ) {
        s->setShuffleState(  pl->guid(), currentPlaylistInterface()->shuffled() );
        s->setRepeatMode( pl->guid(), currentPlaylistInterface()->repeatMode() );
    } else {
        Tomahawk::dynplaylist_ptr dynPl = dynamicPlaylistForInterface( currentPlaylistInterface() );
        if ( !dynPl.isNull() ) {
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

        connect( currentPlaylistInterface().data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ),
                                                    SIGNAL( repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ) );

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

    emit autoUpdateAvailable( currentPage()->canAutoUpdate() );

/*    if ( !currentPage()->showStatsBar() && !currentPage()->showModes() && !currentPage()->showFilter() )
        m_topbar->setVisible( false );
    else
        m_topbar->setVisible( true );*/

    m_infobar->setVisible( currentPage()->showInfoBar() );
    m_infobar->setCaption( currentPage()->title() );
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
    qDebug() << "Destroyed child:" << widget << widget->metaObject()->className();

    bool resetWidget = ( m_stack->currentWidget() == widget );

    for ( int i = 0; i < m_pageHistory.count(); i++ )
    {
        ViewPage* page = m_pageHistory.at( i );
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

        if ( page->widget() == widget && !resetWidget )
        {
            m_pageHistory.removeAt( i );
        }
    }

    m_stack->removeWidget( widget );

    if ( resetWidget )
    {
        historyBack();
    }
}


void
ViewManager::setRepeatMode( Tomahawk::PlaylistInterface::RepeatMode mode )
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
ViewManager::pageForCollection( const collection_ptr& col ) const
{
    return m_collectionViews.value( col ).data();
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
    for ( int i = 0; i < m_pageHistory.count(); i++ )
    {
        ViewPage* page = m_pageHistory.at( i );
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
    return m_pageHistory.isEmpty() ? 0 : m_pageHistory.front();
}


Tomahawk::playlist_ptr
ViewManager::playlistForInterface( Tomahawk::playlistinterface_ptr interface ) const
{
    foreach ( QWeakPointer<PlaylistView> view, m_playlistViews.values() )
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
    foreach ( QWeakPointer<CollectionView> view, m_collectionViews.values() )
    {
        if ( view.data()->playlistInterface() == interface )
        {
            return m_collectionViews.key( view );
        }
    }
    foreach ( QWeakPointer<AlbumView> view, m_collectionAlbumViews.values() )
    {
        if ( view.data()->playlistInterface() == interface )
        {
            return m_collectionAlbumViews.key( view );
        }
    }

    return collection_ptr();
}


bool
ViewManager::isSuperCollectionVisible() const
{
    return ( m_pageHistory.count() &&
           ( currentPage()->playlistInterface() == m_superCollectionView->playlistInterface() ||
             currentPage()->playlistInterface() == m_superAlbumView->playlistInterface() ) );
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

        if ( dynamic_cast< CollectionView* >( page ) )
            m_currentMode = PlaylistInterface::Flat;
        else if ( dynamic_cast< AlbumView* >( page ) )
            m_currentMode = PlaylistInterface::Album;
        else if ( dynamic_cast< ArtistView* >( page ) )
            m_currentMode = PlaylistInterface::Tree;
        else
            return;

        emit modeChanged( (PlaylistInterface::ViewMode)m_currentMode );
    }
}

