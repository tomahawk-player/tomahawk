/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicWidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>

#include "DynamicControlList.h"
#include "playlistview.h"
#include "playlistmodel.h"
#include "trackproxymodel.h"
#include "dynamic/GeneratorInterface.h"
#include "dynamic/GeneratorFactory.h"
#include "pipeline.h"
#include "audioengine.h"
#include "ReadOrWriteWidget.h"

using namespace Tomahawk;

DynamicWidget::DynamicWidget( const Tomahawk::dynplaylist_ptr& playlist, QWidget* parent )
    : QWidget(parent)
    , m_layout( new QVBoxLayout )
    , m_resolveOnNextLoad( false )
    , m_runningOnDemand( false )
    , m_startOnResolved( false )
    , m_songsSinceLastResolved( 0 )
    , m_headerText( 0 )
    , m_headerLayout( 0 )
    , m_modeCombo( 0 )
    , m_generatorCombo( 0 )
    , m_logo( 0 )
    , m_generateButton( 0 )
    , m_controls( 0 )
    , m_splitter( 0 )
    , m_view( 0 )
    , m_model()
{   
    m_headerLayout = new QHBoxLayout;
    m_headerText = new QLabel( tr( "Type:" ), this );
    m_headerLayout->addWidget( m_headerText );
    QComboBox* mode = new QComboBox( this );
    mode->addItem( tr( "On Demand" ), OnDemand );
    mode->addItem( tr( "Static" ), Static );
    connect( mode, SIGNAL( activated( int ) ), this, SLOT( modeChanged( int ) ) );
    m_modeCombo = new ReadOrWriteWidget( mode, playlist->author()->isLocal(), this );
    m_headerLayout->addWidget( m_modeCombo );
    
    QComboBox* gen = new QComboBox( this );
    foreach( const QString& type, GeneratorFactory::types() )
        gen->addItem( type );
    m_generatorCombo = new ReadOrWriteWidget( gen, playlist->author()->isLocal(), this );
    m_headerLayout->addWidget( m_generatorCombo );
    
    m_generateButton = new QPushButton( tr( "Generate" ), this );
    connect( m_generateButton, SIGNAL( clicked( bool ) ), this, SLOT( generateOrStart() ) );
    m_headerLayout->addWidget( m_generateButton );
    
    m_headerLayout->addStretch( 1 );
    
    m_genNumber = new QSpinBox( this );
    m_genNumber->setValue( 15 );
    m_genNumber->setMinimum( 0 );
    m_genNumber->hide();
    
    m_logo = new QLabel( this );
    if( !playlist->generator()->logo().isNull() ) {
        QPixmap p = playlist->generator()->logo().scaledToHeight( m_headerText->height(), Qt::SmoothTransformation );
        m_logo->setPixmap( p );
    }
    m_headerLayout->addWidget(m_logo);
    
    m_layout->addLayout( m_headerLayout );
    
    m_splitter = new AnimatedSplitter( this );
    m_splitter->setOrientation( Qt::Vertical );
    m_splitter->setChildrenCollapsible( false );
    
    m_layout->addWidget( m_splitter );
    m_controls = new DynamicControlList( m_splitter );
    m_model = new PlaylistModel( this );
    m_view = new PlaylistView( this );
    m_view->setModel( m_model );
    
    m_splitter->addWidget( m_controls );
    m_splitter->addWidget( m_view );
    m_splitter->setGreedyWidget( 1 );
    m_splitter->setHandleWidth( 0 );
    
    m_splitter->show( 0, false );
    
    loadDynamicPlaylist( playlist );
    
    setLayout( m_layout );

    connect( m_controls, SIGNAL( controlChanged( Tomahawk::dyncontrol_ptr ) ), this, SLOT( controlChanged( Tomahawk::dyncontrol_ptr ) ), Qt::QueuedConnection );
    connect( m_controls, SIGNAL( controlsChanged() ), this, SLOT( controlsChanged() ), Qt::QueuedConnection );
}

DynamicWidget::~DynamicWidget()
{
}

void DynamicWidget::loadDynamicPlaylist(const Tomahawk::dynplaylist_ptr& playlist)
{
    if( !m_playlist.isNull() ) {
        disconnect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
        disconnect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision) ), this, SLOT(onRevisionLoaded( Tomahawk::DynamicPlaylistRevision) ) );
        disconnect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( onDemandFetched( Tomahawk::query_ptr ) ) );
    }
    
    m_playlist = playlist;
    m_model->loadPlaylist( m_playlist );
    
    if( !m_playlist.isNull() )
        m_controls->setControls( m_playlist->generator(), m_playlist->generator()->controls(), m_playlist->author()->isLocal() );
    qobject_cast<QComboBox*>( m_modeCombo->writableWidget() )->setCurrentIndex( static_cast<int>( playlist->mode() ) );
    
    m_generatorCombo->setWritable( playlist->author()->isLocal() );
    m_generatorCombo->setLabel( qobject_cast< QComboBox* >( m_generatorCombo->writableWidget() )->currentText() );
    m_modeCombo->setWritable( playlist->author()->isLocal() );
    m_modeCombo->setLabel( qobject_cast< QComboBox* >( m_modeCombo->writableWidget() )->currentText() );
       
    applyModeChange( m_playlist->mode() );
    connect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    connect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( onDemandFetched( Tomahawk::query_ptr ) ) );
    
}


void 
DynamicWidget::onRevisionLoaded( const Tomahawk::DynamicPlaylistRevision& rev )
{
    qDebug() << "DynamicWidget::onRevisionLoaded";
    loadDynamicPlaylist( m_playlist );
    if( m_resolveOnNextLoad )
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

void 
DynamicWidget::generateOrStart()
{
    if( m_playlist->mode() == Static ) 
    {
        // get the items from the generator, and put them in the playlist
        m_playlist->generator()->generate( m_genNumber->value() );
    } else if( m_playlist->mode() == OnDemand ) {
        if( m_runningOnDemand == false ) {
            m_runningOnDemand = true;
            m_startOnResolved = true;
            m_playlist->generator()->startOnDemand();
            
            m_generateButton->setText( tr( "Stop" ) );
        } else { // stop
            m_runningOnDemand = false;
            m_startOnResolved = false;
            m_generateButton->setText( tr( "Start" ) );
        }
    }
}

void 
DynamicWidget::modeChanged( int mode )
{
    qDebug() << Q_FUNC_INFO;
    
    m_playlist->setMode( mode );
    applyModeChange( mode );
    controlsChanged();
}

void 
DynamicWidget::applyModeChange( int mode )
{
    if( mode == OnDemand )
    {
        m_generateButton->setText( tr( "Play" ) );
        m_genNumber->hide();
        
        connect( TomahawkApp::instance()->audioEngine(), SIGNAL( loading( Tomahawk::result_ptr ) ), this, SLOT( newTrackLoading() ) );
    } else if( mode == Static ) {
        m_generateButton->setText( tr( "Generate" ) );
        m_genNumber->show();
        if( m_headerLayout->indexOf( m_genNumber ) == -1 )
            m_headerLayout->insertWidget( 4, m_genNumber );
        
        disconnect( TomahawkApp::instance()->audioEngine(), SIGNAL( loading( Tomahawk::result_ptr ) ), this, SLOT( newTrackLoading() ) );
    }
}


void 
DynamicWidget::tracksGenerated( const QList< query_ptr >& queries )
{
    m_playlist->addEntries( queries, m_playlist->currentrevision() );
    m_resolveOnNextLoad = true;
}

void 
DynamicWidget::onDemandFetched( const Tomahawk::query_ptr& track )
{
    connect( track.data(), SIGNAL( resolveFailed() ), this, SLOT( trackResolveFailed() ) );
    connect( track.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ), this, SLOT( trackResolved() ) );
    
    m_model->appendTrack( track );
    Pipeline::instance()->add( track );
}

void
DynamicWidget::trackResolved()
{
    m_songsSinceLastResolved = 0;
    
    if( m_startOnResolved ) {
        m_startOnResolved = false;
        TomahawkApp::instance()->audioEngine()->play();
    }
    
}

void 
DynamicWidget::trackResolveFailed()
{
    m_songsSinceLastResolved++;
    if( m_songsSinceLastResolved < 100 ) {
        m_playlist->generator()->fetchNext();
    }
}

void 
DynamicWidget::newTrackLoading()
{
    if( m_runningOnDemand && m_songsSinceLastResolved == 0 ) { // if we're in dynamic mode and we're also currently idle
        m_playlist->generator()->fetchNext();
    }
}

void DynamicWidget::onDemandFailed()
{
    if( m_runningOnDemand )
        generateOrStart();
}


void 
DynamicWidget::controlsChanged()
{
    // save the current playlist
    if( !m_controls->lastControlDirty() )
        m_playlist->generator()->controls().removeLast();
    m_playlist->createNewRevision();
}

void 
DynamicWidget::controlChanged(const Tomahawk::dyncontrol_ptr& control)
{

}

