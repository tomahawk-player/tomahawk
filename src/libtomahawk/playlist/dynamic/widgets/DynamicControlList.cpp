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

#include "DynamicControlList.h"

#include <QLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPushButton>
#include <QToolButton>
#include <QPainter>

#include "DynamicControlWidget.h"
#include "dynamic/GeneratorInterface.h"
#include "tomahawk/tomahawkapp.h"
#include <QHBoxLayout>

using namespace Tomahawk;

DynamicControlList::DynamicControlList()
    : AnimatedWidget()
    , m_layout( new QVBoxLayout )
    , m_summaryWidget( 0 )
{
    init();
}

DynamicControlList::DynamicControlList( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , m_layout( new QVBoxLayout )
    , m_summaryWidget( 0 )
    , m_isLocal( true )
{
    init();
}

DynamicControlList::DynamicControlList( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, AnimatedSplitter* parent, bool isLocal )
    : AnimatedWidget(parent)
    , m_generator( generator )
    , m_layout( new QVBoxLayout )
    , m_summaryWidget( 0 )
    , m_isLocal( isLocal )
{
    init();
    setControls(  generator, controls, m_isLocal );
}

DynamicControlList::~DynamicControlList()
{

}

void 
DynamicControlList::init()
{
    setLayout( m_layout );
    m_layout->setMargin( 0 );
    m_layout->setSpacing( 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setSizeConstraint( QLayout::SetMinimumSize );
//     setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Ignored );
    splitter()->setStretchFactor( 0, 0 );
    splitter()->setStretchFactor( 1,1 );
    
    m_summaryWidget = new QWidget( this );
    // TODO replace
//     m_summaryWidget->setMinimumHeight( 24 );
//     m_summaryWidget->setMaximumHeight( 24 );
    m_summaryWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_summaryWidget->setLayout( new QVBoxLayout );
    m_summaryWidget->layout()->setMargin( 0 );
    m_summaryWidget->layout()->addWidget( new QLabel( "replace me plz", m_summaryWidget ) );
    
    m_collapseLayout = new QHBoxLayout( this );
    m_collapseLayout->setContentsMargins( 0, 0, 0, 0 );
    m_collapseLayout->setMargin( 0 );
    m_collapseLayout->setSpacing( 0 );
    m_collapse = new QPushButton( tr( "Click to collapse" ), this );
    m_collapseLayout->addWidget( m_collapse );
    m_addControl = new QToolButton( this );
    m_addControl->setIcon( QIcon( RESPATH "images/list-add.png" ) );
    m_addControl->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_addControl->setIconSize( QSize( 16, 16 ) );
    m_addControl->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_addControl->setAutoRaise( true );
    m_addControl->setContentsMargins( 0, 0, 0, 0 );
    m_collapseLayout->addWidget( m_addControl );
    m_collapse->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    
//     connect( m_collapse, SIGNAL( clicked() ), this,  );
    connect( m_addControl, SIGNAL( clicked() ), this, SLOT( addNewControl() ) );
    
    setHiddenSize( m_summaryWidget->size() );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    emit showWidget();
}

void 
DynamicControlList::setControls( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, bool isLocal )
{
    if( !m_controls.isEmpty() ) {
        qDeleteAll( m_controls );
        m_controls.clear();
    }
    m_isLocal = isLocal;
    m_generator = generator;
    if( controls.isEmpty() ) {
        m_controls <<  new DynamicControlWidget( generator->createControl(), isLocal, this );
        connect( m_controls.last(), SIGNAL( removeControl() ), this, SLOT( removeControl() ) );
        connect( m_controls.last(), SIGNAL( changed() ), this, SLOT( controlChanged() ) );
    } else 
    {
        foreach( const dyncontrol_ptr& control, controls ) {
            m_controls << new DynamicControlWidget( control, isLocal, this );
            connect( m_controls.last(), SIGNAL( removeControl() ), this, SLOT( removeControl() ) );
            connect( m_controls.last(), SIGNAL( changed() ), this, SLOT( controlChanged() ) );
        }
    }
    onShown( this );
}

void 
DynamicControlList::onHidden( QWidget* w )
{
    if( w != this )
        return;
    
    AnimatedWidget::onHidden( w );
    
    foreach( DynamicControlWidget* control, m_controls ) {
        m_layout->removeWidget( control );
        control->hide();
    }
    m_layout->addWidget( m_summaryWidget );
    m_summaryWidget->show();
}

void 
DynamicControlList::onShown( QWidget* w )
{
    if( w != this )
        return;
    
    
    m_layout->removeWidget( m_summaryWidget );
    m_layout->removeItem( m_collapseLayout );
    
    m_summaryWidget->hide();
    foreach( DynamicControlWidget* control, m_controls ) {
        m_layout->addWidget( control );
    }
    
    m_layout->addItem( m_collapseLayout );
    m_layout->setStretchFactor( m_collapseLayout, 1 );
    
    AnimatedWidget::onShown( w );
}

void DynamicControlList::addNewControl()
{
    dyncontrol_ptr control = m_generator->createControl();
    m_controls.append( new DynamicControlWidget( control, m_isLocal, this ) );
    m_layout->addWidget( m_controls.last() );
    connect( m_controls.last(), SIGNAL( removeControl() ), this, SLOT( removeControl() ) );
    connect( m_controls.last(), SIGNAL( changed() ), this, SLOT( controlChanged() ) );
    
    emit controlsChanged();
}

void DynamicControlList::removeControl()
{
    DynamicControlWidget* w = qobject_cast<DynamicControlWidget*>( sender() );
    m_layout->removeWidget( w );
    m_controls.removeAll( w );
    
    m_generator->removeControl( w->control() );
    delete w;
    
    emit controlsChanged();
}

void DynamicControlList::controlChanged()
{
    Q_ASSERT( sender() && qobject_cast<DynamicControlWidget*>(sender()) );
    DynamicControlWidget* widget = qobject_cast<DynamicControlWidget*>(sender());
    
    emit controlChanged( widget->control() );
}


void DynamicControlList::paintEvent(QPaintEvent* )
{
}
