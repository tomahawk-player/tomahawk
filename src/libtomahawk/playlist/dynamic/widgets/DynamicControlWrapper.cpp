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

#include "DynamicControlWrapper.h"

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

DynamicControlWrapper::DynamicControlWrapper( const Tomahawk::dyncontrol_ptr& control, QGridLayout* layout, int row, bool isLocal, QWidget* parent )
     : QObject( parent )
     , m_isLocal( isLocal )
     , m_mouseOver( false )
     , m_parent( parent )
     , m_row( row )
     , m_minusButton( 0 )
     , m_control( control )
     , m_typeSelector( 0 )
     , m_matchSelector( 0 )
     , m_entryWidget( 0 )
     , m_layout( layout )
{
    
    qDebug() << "CREATING DYNAMIC CONTROL WRAPPER WITH ROW:" << row << layout;
    
    QComboBox* typeSelector = new QComboBox( parent );
    m_typeSelector = new ReadOrWriteWidget( typeSelector, m_isLocal, m_parent );
    
    m_matchSelector = new ReadOrWriteWidget( control->matchSelector(), m_isLocal, m_parent );
    m_entryWidget = new ReadOrWriteWidget( control->inputField(), m_isLocal, m_parent );
        
    if( m_isLocal )
    {
        m_minusButton = initButton();
        m_minusButton->setIcon( QIcon( RESPATH "images/list-remove.png" ) );
        connect( m_minusButton, SIGNAL( clicked( bool ) ), this, SIGNAL( removeControl() ) );
        
        
        m_plusL = new QStackedLayout;
        m_plusL->setContentsMargins( 0, 0, 0, 0 );
        m_plusL->setMargin( 0 );
        m_plusL->addWidget( m_minusButton );
        m_plusL->addWidget( createDummy( m_minusButton ) ); // :-(
    }
    
    connect( typeSelector, SIGNAL( activated( QString) ), SLOT( typeSelectorChanged( QString ) ) );    
    connect( m_control.data(), SIGNAL( changed() ), this, SIGNAL( changed() ) );
    
    m_layout->addWidget( m_typeSelector, row, 0, Qt::AlignLeft );
    
    if( !control.isNull() ) {
        foreach( const QString& type, control->typeSelectors() )
            typeSelector->addItem( type );
    }
    
    typeSelectorChanged( m_control.isNull() ? "" : m_control->selectedType(), true );
    
    if( m_isLocal )
    {
        m_layout->addLayout( m_plusL, m_row, 3, Qt::AlignCenter );
        m_plusL->setCurrentIndex( 1 );
    }
    
}

DynamicControlWrapper::~DynamicControlWrapper()
{
    // remove the controls widgets from our layout so they are not parented
    // we don't want to auto-delete them since the control should own them
    // if we delete them, then the control will be holding on to null ptrs
    removeFromLayout();
    m_control->inputField()->setParent( 0 );
    m_control->matchSelector()->setParent( 0 );
    
    delete m_typeSelector;
    delete m_matchSelector;
    delete m_entryWidget;
}

dyncontrol_ptr 
DynamicControlWrapper::control() const
{
    return m_control;
}

void 
DynamicControlWrapper::removeFromLayout()
{
    m_layout->removeWidget( m_typeSelector );
    m_layout->removeWidget( m_matchSelector );
    m_layout->removeWidget( m_entryWidget );
    m_layout->removeItem( m_plusL );
}


QToolButton* DynamicControlWrapper::initButton()
{
    QToolButton* btn = new QToolButton( m_parent );
    btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    btn->setIconSize( QSize( 16, 16 ) );
    btn->setToolButtonStyle( Qt::ToolButtonIconOnly );
    btn->setAutoRaise( true );
    btn->setContentsMargins( 0, 0, 0, 0 );
    return btn;
}

QWidget* DynamicControlWrapper::createDummy( QWidget* fromW )
{
    QWidget* dummy = new QWidget( m_parent );
    dummy->setContentsMargins( 0, 0, 0, 0 );
    dummy->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    dummy->setMinimumSize( fromW->size() );
    dummy->setMaximumSize( fromW->size() );
    return dummy;
}


void 
DynamicControlWrapper::typeSelectorChanged( const QString& type, bool firstLoad )
{
    Q_ASSERT( m_layout );
    m_layout->removeWidget( m_matchSelector );
    m_layout->removeWidget( m_entryWidget );
    
    if( m_control->selectedType() != type && !firstLoad )
        m_control->setSelectedType( type );
    
    m_typeSelector->setLabel( type );
    
    QComboBox* typeSel = qobject_cast<QComboBox*>(m_typeSelector->writableWidget());
    if( m_typeSelector->writable() && m_typeSelector->writableWidget() && typeSel ) {
        int idx = typeSel->findText( type );
        if( idx > -1 )
            typeSel->setCurrentIndex( idx );
    }
    
    if( m_control->matchSelector() ) {
        m_matchSelector->setWritableWidget( m_control->matchSelector() );
        m_matchSelector->setLabel( m_control->matchString() );
        m_matchSelector->setWritable( m_isLocal );
        m_layout->addWidget( m_matchSelector, m_row, 1, Qt::AlignCenter );
    }
    if( m_control->inputField() ) {
        m_entryWidget->setWritableWidget( m_control->inputField() );
        m_entryWidget->setLabel( m_control->input() );
        m_entryWidget->setWritable( m_isLocal );
        m_layout->addWidget( m_entryWidget, m_row, 2 );
        
    }
    
    emit changed();
}
/*
void 
DynamicControlWrapper::enterEvent(QEvent* ev)
{
    m_mouseOver = true;
    if( m_isLocal )
        m_plusL->setCurrentIndex( 0 );
    
    if( ev )
        QObject::enterEvent( ev );
}

void 
DynamicControlWrapper::leaveEvent(QEvent* ev)
{
    m_mouseOver = true;
    if( m_isLocal )
        m_plusL->setCurrentIndex( 1 );
    
    if( ev )
        QWidget::leaveEvent( ev );
}
*/

