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

#include "DynamicWidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>

#include "DynamicControlList.h"
#include "dynamic/DynamicModel.h"
#include "trackproxymodel.h"
#include "dynamic/GeneratorInterface.h"
#include "dynamic/GeneratorFactory.h"
#include "pipeline.h"
#include "audio/audioengine.h"
#include "ReadOrWriteWidget.h"
#include "CollapsibleControls.h"
#include "DynamicControlWrapper.h"
#include "playlistmanager.h"
#include "dynamic/DynamicView.h"
#include <qevent.h>
#include "DynamicSetupWidget.h"
#include <QPainter>

#include "audiocontrols.h"
#include "LoadingSpinner.h"

using namespace Tomahawk;

DynamicWidget::DynamicWidget( const Tomahawk::dynplaylist_ptr& playlist, QWidget* parent )
    : QWidget(parent)
    , m_layout( new QVBoxLayout )
    , m_resolveOnNextLoad( false )
    , m_seqRevLaunched( 0 )
    , m_activePlaylist( false )
    , m_setup( 0 )
    , m_runningOnDemand( false )
    , m_controlsChanged( false )
    , m_steering( 0 )
    , m_controls( 0 )
    , m_view( 0 )
    , m_model()
{   
    m_controls = new CollapsibleControls( this );
    m_layout->addWidget( m_controls );
    setContentsMargins( 0, 0, 0, 1 ); // to align the bottom with the bottom of the sourcelist
    
    m_model = new DynamicModel( this );
    m_view = new DynamicView( this );
    m_view->setModel( m_model );
    m_view->setContentsMargins( 0, 0, 0, 0 );
    m_layout->addWidget( m_view, 1 );
    
    connect( m_model, SIGNAL( collapseFromTo( int, int ) ), m_view, SLOT( collapseEntries( int, int ) ) );
    connect( m_model, SIGNAL( trackGenerationFailure( QString ) ), this, SLOT( stationFailed( QString ) ) );    
    
    m_loading = new LoadingSpinner( m_view ); 
    connect( m_model, SIGNAL( tracksAdded() ), m_loading, SLOT( fadeOut() ) );
    
    m_setup = new DynamicSetupWidget( playlist, this );
    m_setup->fadeIn();
    
    connect( m_model, SIGNAL( tracksAdded() ), this, SLOT( tracksAdded() ) );
    
    loadDynamicPlaylist( playlist );
    
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setMargin( 0 );
    m_layout->setSpacing( 0 );
    setLayout( m_layout );

    connect( m_setup, SIGNAL( generatePressed( int ) ), this, SLOT( generate( int ) ) );
    connect( m_setup, SIGNAL( typeChanged( QString ) ), this, SLOT( playlistTypeChanged( QString ) ) );

    layoutFloatingWidgets();

    connect( m_controls, SIGNAL( controlChanged( Tomahawk::dyncontrol_ptr ) ), this, SLOT( controlChanged( Tomahawk::dyncontrol_ptr ) ), Qt::QueuedConnection );
    connect( m_controls, SIGNAL( controlsChanged() ), this, SLOT( controlsChanged() ), Qt::QueuedConnection );

    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), this, SLOT( trackStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( playlistChanged( PlaylistInterface* ) ), this, SLOT( playlistChanged( PlaylistInterface* ) ) );
}

DynamicWidget::~DynamicWidget()
{
}

void 
DynamicWidget::loadDynamicPlaylist( const Tomahawk::dynplaylist_ptr& playlist )
{
    // special case: if we have launched multiple setRevision calls, and the number of controls is different, it means that we're getting an intermediate setRevision
    //  called after the user has already created more revisions. ignore in that case.
    if( m_playlist.data() == playlist.data() && m_seqRevLaunched > 0
        && m_controls->controls().size() != playlist->generator()->controls().size() // different number of controls
        && qAbs( m_playlist->generator()->controls().size() - playlist->generator()->controls().size() ) < m_seqRevLaunched ) { // difference in controls has to be less than how many revisions we launched
        return;
    }
    m_seqRevLaunched = 0;
    
    // if we're being told to load the same dynamic playlist over again, only do it if the controls have a different number
    if( !m_playlist.isNull() && ( m_playlist.data() == playlist.data() ) // same playlist pointer
        && m_playlist->generator()->controls().size() == playlist->generator()->controls().size() ) {
        // we can skip our work. just let the dynamiccontrollist show the difference
        m_controls->setControls( m_playlist, m_playlist->author()->isLocal() );
    
        m_playlist = playlist;
        
        if( !m_runningOnDemand ) {
            m_model->loadPlaylist( m_playlist );
        } else if( !m_controlsChanged ) { // if the controls changed, we already dealt with that and don't want to change station yet
            m_model->changeStation();
        }
        m_controlsChanged = false;
        
        return;
    }
    
    if( !m_playlist.isNull() ) {
        disconnect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
        disconnect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision) ), this, SLOT(onRevisionLoaded( Tomahawk::DynamicPlaylistRevision) ) );
        disconnect( m_playlist->generator().data(), SIGNAL( error( QString, QString ) ), this, SLOT( generatorError( QString, QString ) ) );
        disconnect( m_playlist.data(), SIGNAL( deleted( Tomahawk::dynplaylist_ptr ) ), this, SLOT( deleteLater() ) );
    }
    
    
    m_playlist = playlist;
    m_view->setOnDemand( m_playlist->mode() == OnDemand );
    m_view->setReadOnly( !m_playlist->author()->isLocal() );
    m_model->loadPlaylist( m_playlist );
    m_controlsChanged = false;
    m_setup->setPlaylist( m_playlist );
        
    
    if( !m_playlist->author()->isLocal() ) { // hide controls, as we show the description in the summary
            m_layout->removeWidget( m_controls );
    } else if( m_layout->indexOf( m_controls ) == -1 ) {
        m_layout->insertWidget( 0, m_controls );
    } 
    
    if( m_playlist->mode() == OnDemand && !m_playlist->generator()->controls().isEmpty() )
        showPreview();
    
    if( !m_playlist.isNull() )
        m_controls->setControls( m_playlist, m_playlist->author()->isLocal() );
    
    connect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    connect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_playlist->generator().data(), SIGNAL( error( QString, QString ) ), this, SLOT( generatorError( QString, QString ) ) );
    connect( m_playlist.data(), SIGNAL( deleted( Tomahawk::dynplaylist_ptr ) ), this, SLOT( deleteLater() ) );
}


void 
DynamicWidget::onRevisionLoaded( const Tomahawk::DynamicPlaylistRevision& rev )
{
    qDebug() << "DynamicWidget::onRevisionLoaded";
    loadDynamicPlaylist( m_playlist );
    if( m_resolveOnNextLoad || !m_playlist->author()->isLocal() )
    {
        m_playlist->resolve();
        m_resolveOnNextLoad = false;
    }
}

PlaylistInterface* 
DynamicWidget::playlistInterface() const
{
    return m_view->proxyModel();
}

QSize
DynamicWidget::sizeHint() const
{
    // We want to take up as much room as the animated splitter containing us and the queue editor will allow. So we return a bogus huge sizehint
    //  to avoid having to calculate it which is slow
    return QSize( 5000, 5000 );
}

void 
DynamicWidget::resizeEvent(QResizeEvent* )
{
    layoutFloatingWidgets();
}

void 
DynamicWidget::layoutFloatingWidgets()
{
    if( !m_runningOnDemand ) {
        int x = ( width() / 2 ) - ( m_setup->size().width() / 2 );
        int y = height() - m_setup->size().height() - 40; // padding
        
        m_setup->move( x, y );
    } else if( m_runningOnDemand && m_steering ) {
        int x = ( width() / 2 ) - ( m_steering->size().width() / 2 );
        int y = height() - m_steering->size().height() - 40; // padding
        
        m_steering->move( x, y );
    }
}

void 
DynamicWidget::playlistChanged( PlaylistInterface* pl )
{
    if( pl == static_cast< PlaylistInterface* >( m_view->proxyModel() ) ) { // same playlist
        m_activePlaylist = true;
    } else {
        m_activePlaylist = false;
        
        // user started playing something somewhere else, so give it a rest
        if( m_runningOnDemand ) {
            stopStation( false );
        }
    }
}

void 
DynamicWidget::showEvent(QShowEvent* )
{
    if( !m_playlist.isNull() && !m_runningOnDemand ) {
        m_setup->fadeIn();
    }
}


void 
DynamicWidget::generate( int num )
{
    // get the items from the generator, and put them in the playlist
    m_view->setDynamicWorking( true );
    m_loading->fadeIn();
    m_playlist->generator()->generate( num );
}

void 
DynamicWidget::stationFailed( const QString& msg )
{
    m_view->setDynamicWorking( false );
    m_view->showMessage( msg );
    m_loading->fadeOut();
    
    stopStation( false );
}

void 
DynamicWidget::trackStarted()
{    
    if( m_activePlaylist && !m_playlist.isNull() &&
        m_playlist->mode() == OnDemand && !m_runningOnDemand ) {
        
        startStation();
    }
}

void 
DynamicWidget::tracksAdded()
{
    if( m_playlist->mode() == OnDemand && m_runningOnDemand && m_setup->isVisible() )
        m_setup->fadeOut();
}


void 
DynamicWidget::stopStation( bool stopPlaying )
{
    m_model->stopOnDemand( stopPlaying );
    m_runningOnDemand = false;
    
    // TODO until i add a qwidget interface
    QMetaObject::invokeMethod( m_steering, "fadeOut", Qt::DirectConnection );
    m_setup->fadeIn();
}

void 
DynamicWidget::startStation()
{
    m_runningOnDemand = true;
    m_model->startOnDemand();
    
    m_setup->fadeOut();
    // show the steering controls
    if( m_playlist->generator()->onDemandSteerable() ) {
        // position it horizontally centered, above the botton.
        m_steering = m_playlist->generator()->steeringWidget();
        Q_ASSERT( m_steering );
        
        int x = ( width() / 2 ) - ( m_steering->size().width() / 2 );
        int y = height() - m_steering->size().height() - 40; // padding
        
        m_steering->setParent( this );
        m_steering->move( x, y );
        
        // TODO until i add a qwidget interface
        QMetaObject::invokeMethod( m_steering, "fadeIn", Qt::DirectConnection );
        
        connect( m_steering, SIGNAL( resized() ), this, SLOT( layoutFloatingWidgets() ) );
    }
}

void 
DynamicWidget::playlistTypeChanged( QString )
{
    // TODO
}

void 
DynamicWidget::tracksGenerated( const QList< query_ptr >& queries )
{   
    int limit = -1; // only limit the "preview" of a station
    if( m_playlist->author()->isLocal() && m_playlist->mode() == Static ) {
        m_resolveOnNextLoad = true;
    } else if( m_playlist->mode() == OnDemand )
        limit = 5;
    
    if( m_playlist->mode() != OnDemand )
        m_loading->fadeOut();
    m_model->tracksGenerated( queries, limit );
}


void 
DynamicWidget::controlsChanged()
{
    // controlsChanged() is emitted when a control is added or removed
    // in the case of addition, it's blank by default... so to avoid an error
    // when playing a station just ignore it till we're ready and get a controlChanged()
    m_controlsChanged = true;
    
    if( !m_playlist->author()->isLocal() )
        return;
    m_playlist->createNewRevision();
    m_seqRevLaunched++;
    
    emit descriptionChanged( m_playlist->generator()->sentenceSummary() );
}

void 
DynamicWidget::controlChanged( const Tomahawk::dyncontrol_ptr& control )
{   
    if( !m_playlist->author()->isLocal() )
        return;       
    m_playlist->createNewRevision();
    m_seqRevLaunched++;
    
    showPreview();
    
    emit descriptionChanged( m_playlist->generator()->sentenceSummary() );
}

void 
DynamicWidget::showPreview()
{
    if( m_playlist->mode() == OnDemand && !m_runningOnDemand && m_model->rowCount( QModelIndex() ) == 0 ) { // if this is a not running station, preview matching tracks
        generate( 20 ); // ask for more, we'll filter how many we actually want
    }
}


void 
DynamicWidget::generatorError( const QString& title, const QString& content )
{
    if( m_runningOnDemand ) {
        stopStation( false );
    }
    m_view->setDynamicWorking( false );
    m_loading->fadeOut();
    m_view->showMessageTimeout( title, content );
}

void
DynamicWidget::paintRoundedFilledRect( QPainter& p, QPalette& pal, QRect& r, qreal opacity )
{   
    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );
    p.setOpacity( opacity );
    
    QPen pen( pal.dark().color(), .5 );
    p.setPen( pen );
    p.setBrush( pal.highlight() );
    
    p.drawRoundedRect( r, 10, 10 );
    
    p.setOpacity( opacity + .2 );
    p.setBrush( QBrush() );
    p.setPen( pen );
    p.drawRoundedRect( r, 10, 10 );
}

bool
DynamicWidget::jumpToCurrentTrack()
{
    m_view->scrollTo( m_view->proxyModel()->currentItem(), QAbstractItemView::PositionAtCenter );
    return true;
}
