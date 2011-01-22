/****************************************************************************************
 * Copyright (c) 2010-2011 Leo Franchi <lfranchi@kde.org>                               *
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

#include "CollapsibleControls.h"

#include "DynamicControlList.h"
#include "DynamicControlWrapper.h"
#include "dynamic/GeneratorInterface.h"
#include "dynamic/DynamicControl.h"

#include <QLabel>
#include <QStackedLayout>

using namespace Tomahawk;

CollapsibleControls::CollapsibleControls( QWidget* parent )
    : QWidget( parent )
{
    init();
}

CollapsibleControls::CollapsibleControls( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, bool isLocal, QWidget* parent )
    : QWidget( parent )
{
    init();
    setControls( generator, controls, isLocal );
}

Tomahawk::CollapsibleControls::~CollapsibleControls()
{

}

void 
CollapsibleControls::init()
{
    m_layout = new QStackedLayout;
    setContentsMargins( 0, 0, 0, 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setSpacing( 0 );
    
    m_controls = new Tomahawk::DynamicControlList( this );
    m_layout->addWidget( m_controls );
    
    m_summaryWidget = new QWidget( this );
    // TODO replace
    //     m_summaryWidget->setMinimumHeight( 24 );
    //     m_summaryWidget->setMaximumHeight( 24 );
    m_summaryWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_summaryWidget->setLayout( new QVBoxLayout );
    m_summaryWidget->layout()->setMargin( 0 );
    m_summaryWidget->layout()->addWidget( new QLabel( "replace me plz", m_summaryWidget ) );
    m_layout->addWidget( m_summaryWidget );
    
    m_layout->setCurrentIndex( 0 );
    connect( m_controls, SIGNAL( controlChanged( Tomahawk::dyncontrol_ptr ) ), SIGNAL( controlChanged( Tomahawk::dyncontrol_ptr ) ) );
    connect( m_controls, SIGNAL( controlsChanged() ), SIGNAL( controlsChanged() ) );
    
    setLayout( m_layout );
    
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
}


QList< DynamicControlWrapper* > 
Tomahawk::CollapsibleControls::controls() const
{
    return m_controls->controls();
}

void 
CollapsibleControls::setControls( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, bool isLocal )
{
    m_controls->setControls( generator, controls, isLocal );
}

void 
CollapsibleControls::toggleCollapse()
{

}
