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
#include "dynamic/DynamicModel.h"
#include "trackproxymodel.h"
#include "dynamic/GeneratorInterface.h"
#include "dynamic/GeneratorFactory.h"
#include "pipeline.h"
#include "audio/audioengine.h"
#include "ReadOrWriteWidget.h"
#include "CollapsibleControls.h"
#include "DynamicControlWrapper.h"
#include "dynamic/DynamicView.h"
#include <qevent.h>
#include "DynamicSetupWidget.h"
#include <QPainter>

using namespace Tomahawk;

DynamicWidget::DynamicWidget( const Tomahawk::dynplaylist_ptr& playlist, QWidget* parent )
    : QWidget(parent)
    , m_layout( new QVBoxLayout )
    , m_resolveOnNextLoad( false )
    , m_seqRevLaunched( 0 )
    , m_setup( 0 )
    , m_runningOnDemand( false )
    , m_controlsChanged( false )
    , m_steering( 0 )
    , m_headerText( 0 )
    , m_headerLayout( 0 )
    , m_generatorCombo( 0 )
    , m_logo( 0 )
    , m_generateButton( 0 )
    , m_controls( 0 )
    , m_view( 0 )
    , m_model()
{   
    m_headerLayout = new QHBoxLayout;
    m_headerText = new QLabel( tr( "Type:" ), this );
    m_headerLayout->addWidget( m_headerText );

    QComboBox* gen = new QComboBox( this );
    foreach( const QString& type, GeneratorFactory::types() )
        gen->addItem( type );
    m_generatorCombo = new ReadOrWriteWidget( gen, playlist->author()->isLocal(), this );
    m_headerLayout->addWidget( m_generatorCombo );
    
    m_generateButton = new QPushButton( tr( "Generate" ), this );
    m_generateButton->setAttribute( Qt::WA_LayoutUsesWidgetRect );
    connect( m_generateButton, SIGNAL( clicked( bool ) ), this, SLOT( generate() ) );
    m_headerLayout->addWidget( m_generateButton );
    
    m_headerLayout->addStretch( 1 );
    
    m_genNumber = new QSpinBox( this );
    m_genNumber->setValue( 15 );
    m_genNumber->setMinimum( 0 );
    m_genNumber->hide();
    
    m_logo = new QLabel( this );
    if( !playlist->generator()->logo().isNull() ) {
        QPixmap p = playlist->generator()->logo().scaledToHeight( 16, Qt::SmoothTransformation );
        m_logo->setPixmap( p );
    }
    m_headerLayout->addWidget(m_logo);
    
    m_controls = new CollapsibleControls( this );
    m_layout->addWidget( m_controls );
    
    m_model = new DynamicModel( this );
    m_view = new DynamicView( this );
    m_view->setModel( m_model );
    m_view->setContentsMargins( 0, 0, 0, 0 );
    m_layout->addWidget( m_view, 1 );
    
    connect( m_model, SIGNAL( collapseFromTo( int, int ) ), m_view, SLOT( collapseEntries( int, int ) ), Qt::QueuedConnection );
    connect( m_model, SIGNAL( trackGenerationFailure( QString ) ), m_view, SLOT( showMessage( QString ) ) );    
    
    
    m_setup = new DynamicSetupWidget( playlist, this );
    if( playlist->mode() == Static ) {
        m_setup->hide();
        
        m_layout->addLayout( m_headerLayout );
    } else {
        m_setup->fadeIn();
        
        // hide the widgets, removing them from layout
        // TODO HACK these need to go away, need a good UI design
        m_headerText->hide();
        m_generatorCombo->hide();
        m_generateButton->hide();
        m_genNumber->hide();
        m_logo->hide();
    }
    
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
    }
    
    if( m_playlist.isNull() || m_playlist->mode() != playlist->mode() ) { // update our ui with the appropriate controls
            if( playlist->mode() == Static ) {
                m_setup->hide();
                
                m_layout->insertLayout( 0, m_headerLayout );
            } else {
                m_setup->fadeIn();

                // hide the widgets, removing them from layout
                // TODO HACK these need to go away, need a good UI design
                m_headerText->hide();
                m_generatorCombo->hide();
                m_generateButton->hide();
                m_genNumber->hide();
                m_logo->hide();
                m_layout->removeItem( m_headerLayout );
            }
    }
    
    m_playlist = playlist;
    m_view->setOnDemand( m_playlist->mode() == OnDemand );
    m_view->setReadOnly( !m_playlist->author()->isLocal() );
    m_model->loadPlaylist( m_playlist );
    m_controlsChanged = false;
    m_setup->setPlaylist( m_playlist );
    
    if( !m_playlist.isNull() )
        m_controls->setControls( m_playlist, m_playlist->author()->isLocal() );
    
    
    m_generatorCombo->setWritable( playlist->author()->isLocal() );
    m_generatorCombo->setLabel( qobject_cast< QComboBox* >( m_generatorCombo->writableWidget() )->currentText() );
    
    applyModeChange( m_playlist->mode() );
    connect( m_playlist->generator().data(), SIGNAL( generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    connect( m_playlist.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_playlist->generator().data(), SIGNAL( error( QString, QString ) ), this, SLOT( generatorError( QString, QString ) ) );
    
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
    if( m_playlist->mode() == OnDemand && !m_runningOnDemand ) {
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
DynamicWidget::hideEvent( QHideEvent* ev )
{
    if( m_runningOnDemand ) {
        stopStation();
    }
    QWidget::hideEvent( ev );
}


void 
DynamicWidget::generate( int num )
{
    if( m_playlist->mode() == Static ) 
    {
        // get the items from the generator, and put them in the playlist
        m_playlist->generator()->generate( num == -1 ? m_genNumber->value() : num ); // HACK while in transition
    } else if( m_playlist->mode() == OnDemand ) {

    }
}

void 
DynamicWidget::stopStation()
{
    m_model->stopOnDemand();
    m_runningOnDemand = false;
    
    // TODO until i add a qwidget interface
    QMetaObject::invokeMethod( m_steering, SLOT( fadeOut() ), Qt::DirectConnection );
    
    m_generateButton->setText( tr( "Start" ) );
}

void 
DynamicWidget::startStation()
{
    m_runningOnDemand = true;
    m_model->startOnDemand();
    
    m_generateButton->setText( tr( "Stop" ) );
    
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
        QMetaObject::invokeMethod( m_steering, SLOT( fadeIn() ), Qt::DirectConnection );
        
        connect( m_steering, SIGNAL( resized() ), this, SLOT( layoutFloatingWidgets() ) );
    }
}

void 
DynamicWidget::playlistTypeChanged( QString )
{
    // TODO
}

void 
DynamicWidget::applyModeChange( int mode )
{
    if( mode == OnDemand )
    {
        m_generateButton->setText( tr( "Play" ) );
        m_genNumber->hide();
    } else if( mode == Static ) {
        m_generateButton->setText( tr( "Generate" ) );
        m_genNumber->show();
        if( m_headerLayout->indexOf( m_genNumber ) == -1 )
            m_headerLayout->insertWidget( 4, m_genNumber );
    }
}


void 
DynamicWidget::tracksGenerated( const QList< query_ptr >& queries )
{
    if( m_playlist->author()->isLocal() ) {
        m_playlist->addEntries( queries, m_playlist->currentrevision() );
        m_resolveOnNextLoad = true;
    } else { // read-only, so add tracks only in the GUI, not to the playlist itself
        foreach( const query_ptr& query, queries ) {
            m_model->append( query );
        }
    }
}


void 
DynamicWidget::controlsChanged()
{
    // controlsChanged() is emitted when a control is added or removed
    // in the case of addition, it's blank by default... so to avoid an error
    // when playing a station just ignore it till we're ready and get a controlChanged()
/*    if( m_runningOnDemand )
        m_model->changeStation();*/
    m_controlsChanged = true;
    
    if( !m_playlist->author()->isLocal() )
        return;
    m_playlist->createNewRevision();
    m_seqRevLaunched++;
}

void 
DynamicWidget::controlChanged( const Tomahawk::dyncontrol_ptr& control )
{   
    if( !m_playlist->author()->isLocal() )
        return;       
    m_playlist->createNewRevision();
    m_seqRevLaunched++;
}

void 
DynamicWidget::generatorError( const QString& title, const QString& content )
{
    m_view->showMessageTimeout( title, content );
    
    if( m_runningOnDemand )
        stopStation();
}

void
DynamicWidget::paintRoundedFilledRect( QPainter& p, QPalette& pal, QRect& r, qreal opacity )
{   
    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );
    p.setOpacity( 0.7 );
    
    QPen pen( pal.dark().color(), .5 );
    p.setPen( pen );
    p.setBrush( pal.highlight() );
    
    p.drawRoundedRect( r, 10, 10 );
    
    p.setOpacity( opacity );
    p.setBrush( QBrush() );
    p.setPen( pen );
    p.drawRoundedRect( r, 10, 10 );
}
