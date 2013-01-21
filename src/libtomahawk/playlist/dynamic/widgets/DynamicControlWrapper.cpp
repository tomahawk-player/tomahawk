/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "DynamicControlWrapper.h"

#include "playlist/dynamic/DynamicControl.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QHBoxLayout>
#include <QComboBox>
#include <QLayout>
#include <QToolButton>
#include <QPaintEvent>
#include <QPainter>
#include <QStackedLayout>

using namespace Tomahawk;


DynamicControlWrapper::DynamicControlWrapper( const Tomahawk::dyncontrol_ptr& control, QGridLayout* layout, int row, QWidget* parent )
     : QObject( parent )
     , m_parent( parent )
     , m_row( row )
     , m_minusButton( 0 )
     , m_control( control )
     , m_typeSelector( 0 )
     , m_layout( QPointer< QGridLayout >( layout ) )
{
    m_typeSelector = new QComboBox( m_parent );

    m_matchSelector = QPointer<QWidget>( control->matchSelector() );
    m_entryWidget = QPointer<QWidget>( control->inputField() );

    m_minusButton = initButton( m_parent );
    m_minusButton->setIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::ListRemove ) );
    connect( m_minusButton, SIGNAL( clicked( bool ) ), this, SIGNAL( removeControl() ) );

    m_plusL = new QStackedLayout();
    m_plusL->setContentsMargins( 0, 0, 0, 0 );
    m_plusL->setMargin( 0 );
    m_plusL->addWidget( m_minusButton );
    m_plusL->addWidget( createDummy( m_minusButton, m_parent ) ); // :-(

    connect( m_typeSelector, SIGNAL( activated( QString) ), SLOT( typeSelectorChanged( QString ) ) );
    connect( m_control.data(), SIGNAL( changed() ), this, SIGNAL( changed() ) );

    m_layout.data()->addWidget( m_typeSelector, row, 0, Qt::AlignLeft );

    if( !control.isNull() ) {
        foreach( const QString& type, control->typeSelectors() )
            m_typeSelector->addItem( type );
    }

    typeSelectorChanged( m_control.isNull() ? "" : m_control->selectedType(), true );

    m_layout.data()->addLayout( m_plusL, m_row, 3, Qt::AlignCenter );
    m_plusL->setCurrentIndex( 0 );
}


DynamicControlWrapper::~DynamicControlWrapper()
{
    // remove the controls widgets from our layout so they are not parented
    // we don't want to auto-delete them since the control should own them
    // if we delete them, then the control will be holding on to null ptrs
    removeFromLayout();

    if( !m_entryWidget.isNull() )
        m_control->inputField()->setParent( 0 );
    if( !m_matchSelector.isNull() )
        m_control->matchSelector()->setParent( 0 );

    delete m_typeSelector;
    delete m_minusButton;
}


dyncontrol_ptr
DynamicControlWrapper::control() const
{
    return m_control;
}


void
DynamicControlWrapper::removeFromLayout()
{
    if( m_layout.isNull() )
        return;

    if( !m_matchSelector.isNull() )
        m_layout.data()->removeWidget( m_matchSelector.data() );
    if( !m_entryWidget.isNull() )
        m_layout.data()->removeWidget( m_entryWidget.data() );
    m_layout.data()->removeWidget( m_typeSelector );
    m_layout.data()->removeItem( m_plusL );
}


QToolButton*
DynamicControlWrapper::initButton( QWidget* parent )
{
    QToolButton* btn = new QToolButton( parent );
    btn->setAttribute( Qt::WA_LayoutUsesWidgetRect );
    btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    btn->setIconSize( QSize( 16, 16 ) );
    btn->setToolButtonStyle( Qt::ToolButtonIconOnly );
    btn->setAutoRaise( true );
    btn->setContentsMargins( 0, 0, 0, 0 );
    return btn;
}


QWidget*
DynamicControlWrapper::createDummy( QWidget* fromW, QWidget* parent )
{
    QWidget* dummy = new QWidget( parent );
    dummy->setAttribute( Qt::WA_LayoutUsesWidgetRect );
    dummy->setContentsMargins( 0, 0, 0, 0 );
    dummy->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    dummy->setMinimumSize( fromW->sizeHint() );
    dummy->setMaximumSize( fromW->sizeHint() );
    return dummy;
}


void
DynamicControlWrapper::typeSelectorChanged( const QString& type, bool firstLoad )
{
    Q_ASSERT( !m_layout.isNull() );
    m_layout.data()->removeWidget( m_matchSelector.data() );
    m_layout.data()->removeWidget( m_entryWidget.data() );

    if( m_control->selectedType() != type && !firstLoad )
        m_control->setSelectedType( type );


    int idx = m_typeSelector->findText( type );
    if( idx > -1 )
        m_typeSelector->setCurrentIndex( idx );


    if( m_control->matchSelector() ) {
        m_matchSelector = QPointer<QWidget>( m_control->matchSelector() );
        m_layout.data()->addWidget( m_matchSelector.data(), m_row, 1, Qt::AlignCenter );
        m_matchSelector.data()->show();
    }
    if( m_control->inputField() ) {
        m_entryWidget = QPointer<QWidget>( m_control->inputField() );
        m_layout.data()->addWidget( m_entryWidget.data(), m_row, 2 );
        m_entryWidget.data()->show();
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

