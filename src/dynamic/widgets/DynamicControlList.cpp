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
#include "DynamicControlWidget.h"

using namespace Tomahawk;

DynamicControlList::DynamicControlList()
    : AnimatedWidget()
    , m_summaryWidget( 0 )
{
    init();
}

DynamicControlList::DynamicControlList( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , m_summaryWidget( 0 )
{
    init();
}

DynamicControlList::DynamicControlList( const QList< dyncontrol_ptr >& controls, AnimatedSplitter* parent)
    : AnimatedWidget(parent)
    , m_summaryWidget( 0 )
{
    init();
    setControls( controls );
}

DynamicControlList::~DynamicControlList()
{

}

void 
DynamicControlList::init()
{
    setLayout( new QVBoxLayout );
    layout()->setMargin( 0 );
    layout()->setSpacing( 0 );
    
    m_summaryWidget = new QWidget();
    // TODO replace
    m_summaryWidget->setMaximumHeight( 24 );
    m_summaryWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_summaryWidget->setLayout( new QVBoxLayout );
    m_summaryWidget->layout()->setMargin( 0 );
    m_summaryWidget->layout()->addWidget( new QLabel( "replace me plz", m_summaryWidget ) );
    
    setHiddenSize( m_summaryWidget->size() );
}

void 
DynamicControlList::setControls(const QList< dyncontrol_ptr >& controls)
{
    foreach( const dyncontrol_ptr& control, controls )
        m_controls << new DynamicControlWidget( control, false, this );
}

void 
DynamicControlList::onHidden( QWidget* w )
{
    if( w != this )
        return;
    
    AnimatedWidget::onHidden( w );
    
    foreach( DynamicControlWidget* control, m_controls ) {
        layout()->removeWidget( control );
    }
    layout()->addWidget( m_summaryWidget );
}

void 
DynamicControlList::onShown( QWidget* w )
{
    if( w != this )
        return;
    
    AnimatedWidget::onShown( w );
    
    layout()->removeWidget( m_summaryWidget );
    foreach( DynamicControlWidget* control, m_controls ) {
        layout()->addWidget( control );
        
        control->setShowPlusButton( control == m_controls.last() );
    }
}
