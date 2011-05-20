/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
#include "utils/animatedsplitter.h"
#include "infobar/infobar.h"
#include "topbar/topbar.h"
#include "widgets/infowidgets/sourceinfowidget.h"
#include "widgets/welcomewidget.h"

#include "treemodel.h"
#include "collectionflatmodel.h"
#include "collectionview.h"
#include "playlistmodel.h"
#include "playlistview.h"
#include "queueview.h"
#include "trackproxymodel.h"
#include "trackmodel.h"
#include "artistview.h"
#include "albumview.h"
#include "albumproxymodel.h"
#include "albummodel.h"
#include "sourcelist.h"
#include "tomahawksettings.h"

#include "dynamic/widgets/DynamicWidget.h"

#include "widgets/welcomewidget.h"
#include "widgets/infowidgets/sourceinfowidget.h"
#include "widgets/newplaylistwidget.h"

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
    , m_currentMode( PlaylistInterface::Tree )
{
    s_instance = this;

    setHistoryPosition( -1 );
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
    m_queueView->queue()->setPlaylistModel( m_queueModel );
    AudioEngine::instance()->setQueue( m_queueView->queue()->proxyModel() );

    m_splitter->addWidget( m_queueView );
    m_splitter->hide( 1, false );

    m_widget->layout()->addWidget( m_infobar );
    m_widget->layout()->addWidget( m_topbar );
    m_widget->layout()->addWidget( line );
    m_widget->layout()->addWidget( m_splitter );

    m_superCollectionView = new ArtistView();
    m_superCollectionModel = new TreeModel( m_superCollectionView );
    m_superCollectionView->setModel( m_superCollectionModel );
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

    connect( AudioEngine::instance(), SIGNAL( playlistChanged( PlaylistInterface* ) ), this, SLOT( playlistInterfaceChanged( PlaylistInterface* ) ) );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );

    connect( m_topbar, SIGNAL( filterTextChanged( QString ) ),
                         SLOT( setFilter( QString ) ) );

    connect( m_topbar, SIGNAL( flatMode() ),
                         SLOT( setTableMode() ) );

    connect( m_topbar, SIGNAL( artistMode() ),
                         SLOT( setTreeMode() ) );

    connect( m_topbar, SIGNAL( albumMode() ),
                         SLOT( setAlbumMode() ) );
}


ViewManager::~ViewManager()
{
    delete m_widget;
}


PlaylistView*
ViewManager::queue() const
{
    return m_queueView->queue();
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


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::playlist_ptr& playlist )
{
    PlaylistView* view;
    if ( !m_playlistViews.contains( playlist ) )
    {
        view = createPageForPlaylist( playlist );
    }
    else
    {
        view = m_playlistViews.value( playlist );
    }

    setPage( view );

    emit numSourcesChanged( SourceList::instance()->count() );

    return view;
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::dynplaylist_ptr& playlist )
{
    if ( !m_dynamicWidgets.contains( playlist ) )
    {
       m_dynamicWidgets[ playlist ] = new Tomahawk::DynamicWidget( playlist, m_stack );

       connect( playlist.data(), SIGNAL( deleted( Tomahawk::dynplaylist_ptr ) ), this, SLOT( onDynamicDeleted( Tomahawk::dynplaylist_ptr ) ) );

       playlist->resolve();
    }

    setPage( m_dynamicWidgets.value( playlist ) );

    if ( playlist->mode() == Tomahawk::OnDemand )
        m_queueView->hide();
    else
        m_queueView->show();

    emit numSourcesChanged( SourceList::instance()->count() );

    return m_dynamicWidgets.value( playlist );
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::artist_ptr& artist )
{
    PlaylistView* view;

    if ( !m_artistViews.contains( artist ) )
    {
        view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setPlaylistModel( model );
        view->setFrameShape( QFrame::NoFrame );
        view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        model->append( artist );

        m_artistViews.insert( artist, view );
    }
    else
    {
        view = m_artistViews.value( artist );
    }

    setPage( view );
    emit numSourcesChanged( 1 );

    return view;
}


Tomahawk::ViewPage*
ViewManager::show( const Tomahawk::album_ptr& album )
{
    PlaylistView* view;
    if ( !m_albumViews.contains( album ) )
    {
        view = new PlaylistView();
        PlaylistModel* model = new PlaylistModel();
        view->setPlaylistModel( model );
        view->setFrameShape( QFrame::NoFrame );
        view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
        model->append( album );

        m_albumViews.insert( album, view );
    }
    else
    {
        view = m_albumViews.value( album );
    }

    setPage( view );
    emit numSourcesChanged( 1 );

    return view;
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
        if ( !m_collectionViews.contains( collection ) )
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
            view = m_collectionViews.value( collection );
        }

        shown = view;
        setPage( view );
    }

    if ( m_currentMode == PlaylistInterface::Tree )
    {
        ArtistView* view;
        if ( !m_treeViews.contains( collection ) )
        {
            view = new ArtistView();
            TreeModel* model = new TreeModel();
            view->setModel( model );
            view->setFrameShape( QFrame::NoFrame );
            view->setAttribute( Qt::WA_MacShowFocusRect, 0 );

            model->addCollection( collection );

            m_treeViews.insert( collection, view );
        }
        else
        {
            view = m_treeViews.value( collection );
        }

        shown = view;
        setPage( view );
    }

    if ( m_currentMode == PlaylistInterface::Album )
    {
        AlbumView* aview;
        if ( !m_collectionAlbumViews.contains( collection ) )
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
            aview = m_collectionAlbumViews.value( collection );
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
    if ( !m_sourceViews.contains( source ) )
    {
        swidget = new SourceInfoWidget( source );
        m_sourceViews.insert( source, swidget );
    }
    else
    {
        swidget = m_sourceViews.value( source );
    }

    setPage( swidget );
    emit numSourcesChanged( 1 );

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

    m_superCollectionModel->setTitle( tr( "All available tracks" ) );
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
ViewManager::playlistInterfaceChanged( PlaylistInterface* interface )
{
    playlist_ptr pl = playlistForInterface( interface );
    if ( !pl.isNull() )
    {
        TomahawkSettings::instance()->appendRecentlyPlayedPlaylist( pl );
    } else
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
ViewManager::showQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "showQueue", Qt::QueuedConnection );
        return;
    }

    m_splitter->show( 1 );
}


void
ViewManager::hideQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "hideQueue", Qt::QueuedConnection );
        return;
    }

    m_splitter->hide( 1 );
}


void
ViewManager::historyBack()
{
    if ( m_historyPosition < 1 )
        return;

    showHistory( m_historyPosition - 1 );
}


void
ViewManager::historyForward()
{
    if ( m_historyPosition >= m_pageHistory.count() - 1 )
        return;

    showHistory( m_historyPosition + 1 );
}


void
ViewManager::showHistory( int historyPosition )
{
    if ( historyPosition < 0 || historyPosition >= m_pageHistory.count() )
    {
        qDebug() << "History position out of bounds!" << historyPosition << m_pageHistory.count();
        Q_ASSERT( false );
        return;
    }

    setHistoryPosition( historyPosition );
    ViewPage* page = m_pageHistory.at( historyPosition );
    qDebug() << "Showing page after a deleting:" << page->widget()->metaObject()->className();
    setPage( page, false );
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
ViewManager::setPage( ViewPage* page, bool trackHistory )
{
    if ( !page )
        return;

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
        m_pageHistory << page;
        setHistoryPosition( m_pageHistory.count() - 1 );
    }

    if ( !playlistForInterface( currentPlaylistInterface() ).isNull() )
        emit playlistActivated( playlistForInterface( currentPlaylistInterface() ) );

    else if ( dynamicPlaylistForInterface( currentPlaylistInterface() ) )
        emit dynamicPlaylistActivated( dynamicPlaylistForInterface( currentPlaylistInterface() ) );
    else if ( collectionForInterface( currentPlaylistInterface() ) )
        emit collectionActivated( collectionForInterface( currentPlaylistInterface() ) );
    else if ( isSuperCollectionVisible() )
        emit superCollectionActivated();
    else if( isNewPlaylistPageVisible() )
        emit newPlaylistActivated();
    /* TODO refactor. now we have rows in the sourcetreeview that are connected to pages, e.g. Stations, Recently Updated, etc
    else if ( !currentPlaylistInterface() )
        emit tempPageActivated();*/

    qDebug() << "View page shown:" << page->title();
    emit viewPageActivated( page );

    if ( !AudioEngine::instance()->playlist() )
        AudioEngine::instance()->setPlaylist( currentPlaylistInterface() );

    // UGH!
    if ( QObject* obj = dynamic_cast< QObject* >( currentPage() ) )
    {
        // if the signal exists (just to hide the qobject runtime warning...)
        if( obj->metaObject()->indexOfSignal( "descriptionChanged(QString)" ) > -1 )
            connect( obj, SIGNAL( descriptionChanged( QString ) ), m_infobar, SLOT( setDescription( QString ) ), Qt::UniqueConnection );

        if( obj->metaObject()->indexOfSignal( "nameChanged(QString)" ) > -1 )
            connect( obj, SIGNAL( nameChanged( QString ) ), m_infobar, SLOT( setCaption( QString ) ), Qt::UniqueConnection );

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
        disconnect( currentPlaylistInterface()->object(), SIGNAL( sourceTrackCountChanged( unsigned int ) ),
                    this,                                 SIGNAL( numTracksChanged( unsigned int ) ) );

        disconnect( currentPlaylistInterface()->object(), SIGNAL( trackCountChanged( unsigned int ) ),
                    this,                                 SIGNAL( numShownChanged( unsigned int ) ) );

        disconnect( currentPlaylistInterface()->object(), SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
                    this,                                 SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

        disconnect( currentPlaylistInterface()->object(), SIGNAL( shuffleModeChanged( bool ) ),
                    this,                                 SIGNAL( shuffleModeChanged( bool ) ) );
    }
}


void
ViewManager::updateView()
{
    if ( currentPlaylistInterface() )
    {
        connect( currentPlaylistInterface()->object(), SIGNAL( sourceTrackCountChanged( unsigned int ) ),
                                                       SIGNAL( numTracksChanged( unsigned int ) ) );

        connect( currentPlaylistInterface()->object(), SIGNAL( trackCountChanged( unsigned int ) ),
                                                       SIGNAL( numShownChanged( unsigned int ) ) );

        connect( currentPlaylistInterface()->object(), SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
                                                       SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ) );

        connect( currentPlaylistInterface()->object(), SIGNAL( shuffleModeChanged( bool ) ),
                                                       SIGNAL( shuffleModeChanged( bool ) ) );

        m_topbar->setFilter( currentPlaylistInterface()->filter() );
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

    if ( currentPage()->queueVisible() )
        m_queueView->show();
    else
        m_queueView->hide();

    emit statsAvailable( currentPage()->showStatsBar() );
    emit modesAvailable( currentPage()->showModes() );
    emit filterAvailable( currentPage()->showFilter() );

    if ( !currentPage()->showStatsBar() && !currentPage()->showModes() && !currentPage()->showFilter() )
        m_topbar->setVisible( false );
    else
        m_topbar->setVisible( true );

    m_infobar->setCaption( currentPage()->title() );
    m_infobar->setDescription( currentPage()->description() );
    m_infobar->setPixmap( currentPage()->pixmap() );
}


void
ViewManager::onWidgetDestroyed( QWidget* widget )
{
    qDebug() << "Destroyed child:" << widget << widget->metaObject()->className();

    bool resetWidget = ( m_stack->currentWidget() == widget );

    for ( int i = 0; i < m_pageHistory.count(); i++ )
    {
        ViewPage* page = m_pageHistory.at( i );

        if ( !playlistForInterface( page->playlistInterface() ).isNull() )
        {
            m_playlistViews.remove( playlistForInterface( page->playlistInterface() ) );
        }
        if ( !dynamicPlaylistForInterface( page->playlistInterface() ).isNull() )
        {
            m_dynamicWidgets.remove( dynamicPlaylistForInterface( page->playlistInterface() ) );
        }

        if ( page->widget() == widget )
        {
            m_pageHistory.removeAt( i );
            if ( m_historyPosition > i )
                m_historyPosition--;
            break;
        }
    }

    m_stack->removeWidget( widget );

    if ( resetWidget )
    {
        if ( m_pageHistory.count() )
            showHistory( m_pageHistory.count() - 1 );
    }
}


void
ViewManager::setRepeatMode( PlaylistInterface::RepeatMode mode )
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
ViewManager::createPlaylist( const Tomahawk::source_ptr& src,
                                 const QVariant& contents )
{
    Tomahawk::playlist_ptr p = Tomahawk::playlist_ptr( new Tomahawk::Playlist( src ) );
    QJson::QObjectHelper::qvariant2qobject( contents.toMap(), p.data() );
    p->reportCreated( p );
}


void
ViewManager::createDynamicPlaylist( const Tomahawk::source_ptr& src,
                                        const QVariant& contents )
{
    Tomahawk::dynplaylist_ptr p = Tomahawk::dynplaylist_ptr( new Tomahawk::DynamicPlaylist( src, contents.toMap().value( "type", QString() ).toString()  ) );
    QJson::QObjectHelper::qvariant2qobject( contents.toMap(), p.data() );
    p->reportCreated( p );
}

ViewPage*
ViewManager::pageForCollection( const collection_ptr& col ) const
{
    return m_collectionViews.value( col, 0 );
}

ViewPage*
ViewManager::pageForDynPlaylist(const dynplaylist_ptr& pl) const
{
    return m_dynamicWidgets.value( pl, 0 );
}

ViewPage*
ViewManager::pageForPlaylist(const playlist_ptr& pl) const
{
    return m_playlistViews.value( pl, 0 );
}


ViewPage*
ViewManager::pageForInterface( PlaylistInterface* interface ) const
{
    for ( int i = 0; i < m_pageHistory.count(); i++ )
    {
        ViewPage* page = m_pageHistory.at( i );
        if ( page->playlistInterface() == interface )
            return page;
    }

    return 0;
}


int
ViewManager::positionInHistory( ViewPage* page ) const
{
    for ( int i = 0; i < m_pageHistory.count(); i++ )
    {
        if ( page == m_pageHistory.at( i ) )
            return i;
    }

    return -1;
}


PlaylistInterface*
ViewManager::currentPlaylistInterface() const
{
    if ( currentPage() )
        return currentPage()->playlistInterface();
    else
        return 0;
}


Tomahawk::ViewPage*
ViewManager::currentPage() const
{
    if ( m_historyPosition >= 0 )
        return m_pageHistory.at( m_historyPosition );
    else
        return 0;
}


void
ViewManager::setHistoryPosition( int position )
{
    m_historyPosition = position;

    emit historyBackAvailable( m_historyPosition > 0 );
    emit historyForwardAvailable( m_historyPosition < m_pageHistory.count() - 1 );
}


Tomahawk::playlist_ptr
ViewManager::playlistForInterface( PlaylistInterface* interface ) const
{
    foreach ( PlaylistView* view, m_playlistViews.values() )
    {
        if ( view->playlistInterface() == interface )
        {
            return m_playlistViews.key( view );
        }
    }

    return playlist_ptr();
}


Tomahawk::dynplaylist_ptr
ViewManager::dynamicPlaylistForInterface( PlaylistInterface* interface ) const
{
    foreach ( DynamicWidget* view, m_dynamicWidgets.values() )
    {
        if ( view->playlistInterface() == interface )
        {
            return m_dynamicWidgets.key( view );
        }
    }

    return dynplaylist_ptr();
}


Tomahawk::collection_ptr
ViewManager::collectionForInterface( PlaylistInterface* interface ) const
{
    foreach ( CollectionView* view, m_collectionViews.values() )
    {
        if ( view->playlistInterface() == interface )
        {
            return m_collectionViews.key( view );
        }
    }
    foreach ( AlbumView* view, m_collectionAlbumViews.values() )
    {
        if ( view->playlistInterface() == interface )
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
    }
}


void
ViewManager::onPlayClicked()
{
    emit playClicked();
}


void
ViewManager::onPauseClicked()
{
    emit pauseClicked();
}
