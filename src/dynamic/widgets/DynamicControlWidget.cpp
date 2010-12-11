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

#include "DynamicControlWidget.h"

#include "tomahawk/tomahawkapp.h"
#include "dynamic/DynamicControl.h"

#include <QHBoxLayout>
#include <QComboBox>
#include <QLayout>
#include <QToolButton>
#include <QPaintEvent>
#include <QPainter>

using namespace Tomahawk;

DynamicControlWidget::DynamicControlWidget( const Tomahawk::dyncontrol_ptr& control, bool showPlus, bool showCollapse, QWidget* parent )
     : QWidget(parent)
     , m_showPlus( showPlus )
     , m_showCollapse( showCollapse )
     , m_plusButton( 0 )
     , m_collapseButton( 0 )
     , m_control( control )
     , m_typeSelector( 0 )
     , m_layout( 0 )
{
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    
    m_layout = new QHBoxLayout;
    m_typeSelector = new QComboBox( this );
    
    m_layout->setMargin( 0 );
    setContentsMargins( 0, 0, 0, 0 );
    
    m_plusButton= new QToolButton( this );
    m_plusButton->setIcon( QIcon( RESPATH "images/list-add.png" ) );
    m_plusButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_plusButton->setIconSize( QSize( 16, 16 ) );
    m_plusButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_plusButton->setAutoRaise( true );
    m_plusButton->setContentsMargins( 0, 0, 0, 0 );
    m_plusButton->hide();
    
    m_collapseButton= new QToolButton( this );
    m_collapseButton->setIcon( QIcon( RESPATH "images/arrow-up-double.png" ) );
    m_collapseButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_collapseButton->setIconSize( QSize( 16, 16 ) );
    m_collapseButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_collapseButton->setAutoRaise( true );
    m_collapseButton->setContentsMargins( 0, 0, 0, 0 );
    m_collapseButton->hide();
    
    connect( m_typeSelector, SIGNAL( currentIndexChanged( QString ) ), SLOT( typeSelectorChanged( QString ) ) );
    
    if( !control.isNull() ) {
        foreach( const QString& type, control->typeSelectors() )
            m_typeSelector->addItem( type );
        
        m_layout->insertWidget( 0, m_typeSelector, 0, Qt::AlignLeft );
        m_layout->insertWidget( 1, m_control->matchSelector(), 0, Qt::AlignCenter );
        m_layout->insertWidget( 2, m_control->inputField(), 1, Qt::AlignAbsolute );
    }
    
    if( m_showCollapse ) {
        m_layout->insertWidget( 3, m_collapseButton, 0, Qt::AlignRight );
        m_plusButton->show();
    }
    if( m_showPlus ) {
        m_layout->insertWidget( 4, m_plusButton, 0, Qt::AlignRight );
        m_plusButton->show();
    }
    
    setLayout( m_layout );
}

DynamicControlWidget::~DynamicControlWidget()
{

}

void 
DynamicControlWidget::typeSelectorChanged( QString type )
{
    Q_ASSERT( m_layout );
    // remove the two widgets, change the control,and re-add the new ones
    if( m_layout->count() >= 4 )
    {
        m_layout->removeWidget( m_control->matchSelector() );
        m_layout->removeWidget( m_control->inputField() );
    }
    
    m_control->setSelectedType( type );
    m_layout->insertWidget( 1, m_control->matchSelector(), 0, Qt::AlignCenter );
    m_layout->insertWidget( 2, m_control->inputField(), 1, Qt::AlignAbsolute );
}

void 
DynamicControlWidget::setShowPlusButton(bool show)
{
    
    if( m_showPlus != show ) {
        if( show ) {
            m_layout->insertWidget( m_layout->count() - 1, m_plusButton, 0, Qt::AlignRight );
            m_plusButton->show();
        } else {
            m_layout->removeWidget( m_plusButton );
            m_plusButton->hide();
        }
    }
    
    m_showPlus = show;
}

bool 
DynamicControlWidget::showPlusButton() const
{
    return m_showPlus;
}

void 
DynamicControlWidget::setShowCollapseButton(bool show)
{
    
    if( m_showCollapse != show ) {
        if( show ) {
            m_layout->insertWidget( 3, m_collapseButton, 0, Qt::AlignRight );
            m_collapseButton->show();
        } else {
            m_layout->removeWidget( m_collapseButton );
            m_collapseButton->hide();
        }
    }
    
    m_showCollapse = show;
}

void 
DynamicControlWidget::paintEvent(QPaintEvent* e)
{
    QPainter p;
    p.fillRect( e->rect(), Qt::yellow );
}

