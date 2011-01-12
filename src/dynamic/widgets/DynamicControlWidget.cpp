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
#include <qstackedlayout.h>

using namespace Tomahawk;

DynamicControlWidget::DynamicControlWidget( const Tomahawk::dyncontrol_ptr& control, bool showPlus, bool showMinus, bool showCollapse, QWidget* parent )
     : QWidget(parent)
     , m_showPlus( showPlus )
     , m_showMinus( showMinus )
     , m_showCollapse( showCollapse )
     , m_plusButton( 0 )
     , m_minusButton( 0 )
     , m_collapseButton( 0 )
     , m_control( control )
     , m_typeSelector( 0 )
     , m_layout( 0 )
{
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setMouseTracking( true );
    
    m_layout = new QHBoxLayout;
    m_typeSelector = new QComboBox( this );
    
    m_layout->setMargin( 0 );
    m_layout->setSpacing( 0 );
    setContentsMargins( 0, 0, 0, 0 );
    
    m_minusButton = initButton();
    m_minusButton->setIcon( QIcon( RESPATH "images/list-remove.png" ) );
    connect( m_minusButton, SIGNAL( clicked( bool ) ), this, SIGNAL( removeControl() ) );
    
    m_plusButton = initButton();
    m_plusButton->setIcon( QIcon( RESPATH "images/list-add.png" ) );
    connect( m_plusButton, SIGNAL( clicked( bool ) ), this, SIGNAL( addNewControl() ) );
    m_plusL = new QStackedLayout;
    m_plusL->setContentsMargins( 0, 0, 0, 0 );
    m_plusL->addWidget( m_plusButton );
    m_plusL->addWidget( m_minusButton );
    m_plusL->addWidget( createDummy( m_plusButton ) ); // :-(
    m_plusL->setCurrentIndex( 2 );
    
    m_collapseButton = initButton();
    m_collapseButton->setIcon( QIcon( RESPATH "images/arrow-up-double.png" ) );
    m_collapseL = new QStackedLayout;
    m_collapseL->setContentsMargins( 0, 0, 0, 0 );
    m_collapseL->addWidget( m_collapseButton );
    m_collapseL->addWidget( createDummy( m_collapseButton ) ); // :-(
    m_collapseL->setCurrentIndex( 1 );
    
    connect( m_collapseButton, SIGNAL( clicked( bool ) ), this, SIGNAL( collapse() ) );
    connect( m_typeSelector, SIGNAL( activated( QString) ), SLOT( typeSelectorChanged( QString ) ) );    
    connect( m_control.data(), SIGNAL( changed() ), this, SIGNAL( changed() ) );
    
    m_layout->addWidget( m_typeSelector, 0, Qt::AlignLeft );
    
    if( !control.isNull() ) {
        foreach( const QString& type, control->typeSelectors() )
            m_typeSelector->addItem( type );
    }
    
    typeSelectorChanged( m_control.isNull() ? "" : m_control->selectedType(), true );
       
    m_layout->addLayout( m_collapseL, 0 );
    m_layout->addLayout( m_plusL, 0 );
    
    if( m_showCollapse )
        m_collapseL->setCurrentIndex( 0 );
    if( m_showPlus )
        m_plusL->setCurrentIndex( 0 );
    if( m_showMinus )
        m_plusL->setCurrentIndex( 1 );
    
    setLayout( m_layout );
}

DynamicControlWidget::~DynamicControlWidget()
{
    // remove the controls widgets from our layout so they are not parented
    // we don't want to auto-delete them since the control should own them
    // if we delete them, then the control will be holding on to null ptrs
    m_layout->removeWidget( m_control->inputField() );
    m_control->inputField()->setParent( 0 );
    m_layout->removeWidget( m_control->matchSelector() );
    m_control->matchSelector()->setParent( 0 );
}

dyncontrol_ptr DynamicControlWidget::control() const
{
    return m_control;
}


QToolButton* DynamicControlWidget::initButton()
{
    QToolButton* btn = new QToolButton( this );
    btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    btn->setIconSize( QSize( 16, 16 ) );
    btn->setToolButtonStyle( Qt::ToolButtonIconOnly );
    btn->setAutoRaise( true );
    btn->setContentsMargins( 0, 0, 0, 0 );
    return btn;
}

QWidget* DynamicControlWidget::createDummy( QWidget* fromW )
{
    QWidget* dummy = new QWidget( this );
    dummy->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    dummy->setMinimumSize( fromW->size() );
    dummy->setMaximumSize( fromW->size() );
    return dummy;
}


void 
DynamicControlWidget::typeSelectorChanged( const QString& type, bool firstLoad )
{
    Q_ASSERT( m_layout );
    m_layout->removeWidget( m_control->matchSelector() );
    m_layout->removeWidget( m_control->inputField() );
    
    if( m_control->selectedType() == type && !firstLoad )
        m_control->setSelectedType( type );
    
    if( m_control->matchSelector() ) {
        m_layout->insertWidget( 1, m_control->matchSelector(), 0 );
        m_control->matchSelector()->show();
    }
    if( m_control->inputField() ) {
        m_layout->insertWidget( 2, m_control->inputField(), 1  );
        m_control->inputField()->show();
    }
    
    emit changed();
}

void 
DynamicControlWidget::setShowPlusButton(bool show)
{
    
    if( m_showPlus != show ) {
        show ? m_plusL->setCurrentIndex( 0 ) : m_plusL->setCurrentIndex( 2 );
    }
    
    m_showPlus = show;
}


void
DynamicControlWidget::setShowCollapseButton(bool show)
{
    if( m_showCollapse != show ) {
        show ? m_collapseL->setCurrentIndex( 0 ) : m_collapseL->setCurrentIndex( 1 );
    }
    
    m_showCollapse = show;
}

void
DynamicControlWidget::setShowMinusButton(bool show)
{   
    m_showMinus = show;
}

void 
DynamicControlWidget::enterEvent(QEvent* ev)
{
    if( m_showMinus )
        m_plusL->setCurrentIndex( 1 );
    
    if( ev )
        QWidget::enterEvent( ev );
}

void 
DynamicControlWidget::leaveEvent(QEvent* ev)
{
    if( m_showMinus )
        m_plusL->setCurrentIndex( 2 );
    
    if( ev )
        QWidget::leaveEvent( ev );
}


void 
DynamicControlWidget::paintEvent(QPaintEvent* )
{
}

