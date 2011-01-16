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
#include "dynamic/widgets/ReadOrWriteWidget.h"

#include <QHBoxLayout>
#include <QComboBox>
#include <QLayout>
#include <QToolButton>
#include <QPaintEvent>
#include <QPainter>
#include <qstackedlayout.h>

using namespace Tomahawk;

DynamicControlWidget::DynamicControlWidget( const Tomahawk::dyncontrol_ptr& control, bool showPlus, bool showMinus, bool showCollapse, bool isLocal, QWidget* parent )
     : QWidget(parent)
     , m_showPlus( showPlus )
     , m_showMinus( showMinus )
     , m_showCollapse( showCollapse )
     , m_isLocal( isLocal )
     , m_plusButton( 0 )
     , m_minusButton( 0 )
     , m_collapseButton( 0 )
     , m_control( control )
     , m_typeSelector( 0 )
     , m_matchSelector( 0 )
     , m_entryWidget( 0 )
     , m_layout( 0 )
{
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setMouseTracking( true );
    
    m_layout = new QHBoxLayout;
    QComboBox* typeSelector = new QComboBox( this );
    m_typeSelector = new ReadOrWriteWidget( typeSelector, m_isLocal, this );
    
    m_matchSelector = new ReadOrWriteWidget( control->matchSelector(), m_isLocal, this );
    m_entryWidget = new ReadOrWriteWidget( control->inputField(), m_isLocal, this );
    
    m_layout->setMargin( 0 );
    m_layout->setSpacing( 0 );
    setContentsMargins( 0, 0, 0, 0 );
    
    if( m_isLocal )
    {
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
    }
    
    m_collapseButton = initButton();
    m_collapseButton->setIcon( QIcon( RESPATH "images/arrow-up-double.png" ) );
    m_collapseL = new QStackedLayout;
    m_collapseL->setContentsMargins( 0, 0, 0, 0 );
    m_collapseL->addWidget( m_collapseButton );
    m_collapseL->addWidget( createDummy( m_collapseButton ) ); // :-(
    m_collapseL->setCurrentIndex( 1 );
    
    connect( m_collapseButton, SIGNAL( clicked( bool ) ), this, SIGNAL( collapse() ) );
    connect( typeSelector, SIGNAL( activated( QString) ), SLOT( typeSelectorChanged( QString ) ) );    
    connect( m_control.data(), SIGNAL( changed() ), this, SIGNAL( changed() ) );
    
    m_layout->addWidget( m_typeSelector, 0, Qt::AlignLeft );
    
    if( !control.isNull() ) {
        foreach( const QString& type, control->typeSelectors() )
            typeSelector->addItem( type );
    }
    
    typeSelectorChanged( m_control.isNull() ? "" : m_control->selectedType(), true );
    
    m_layout->addLayout( m_collapseL, 0 );
    
    if( m_isLocal )
    {
        m_layout->addLayout( m_plusL, 0 );
        
        if( m_showPlus )
            m_plusL->setCurrentIndex( 0 );
        if( m_showMinus )
            m_plusL->setCurrentIndex( 1 );
    }
    
    if( m_showCollapse )
        m_collapseL->setCurrentIndex( 0 );
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
    m_layout->removeWidget( m_matchSelector );
    m_layout->removeWidget( m_entryWidget );
    
    if( m_control->selectedType() == type && !firstLoad )
        m_control->setSelectedType( type );
    
    m_typeSelector->setLabel( type );
    if( m_control->matchSelector() ) {
        m_matchSelector->setWritableWidget( m_control->matchSelector() );
        m_matchSelector->setLabel( m_control->match() );
        m_matchSelector->setWritable( m_isLocal );
        m_layout->insertWidget( 1, m_matchSelector, 0 );
    }
    if( m_control->inputField() ) {
        m_entryWidget->setWritableWidget( m_control->inputField() );
        m_entryWidget->setLabel( m_control->input() );
        m_entryWidget->setWritable( m_isLocal );
        m_layout->insertWidget( 2, m_entryWidget, 1  );
    }
    
    emit changed();
}

void 
DynamicControlWidget::setShowPlusButton(bool show)
{
    
    if( m_showPlus != show && m_isLocal ) {
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
    if( m_showMinus && m_isLocal )
        m_plusL->setCurrentIndex( 1 );
    
    if( ev )
        QWidget::enterEvent( ev );
}

void 
DynamicControlWidget::leaveEvent(QEvent* ev)
{
    if( m_showMinus && m_isLocal )
        m_plusL->setCurrentIndex( 2 );
    
    if( ev )
        QWidget::leaveEvent( ev );
}


void 
DynamicControlWidget::paintEvent(QPaintEvent* )
{
}

