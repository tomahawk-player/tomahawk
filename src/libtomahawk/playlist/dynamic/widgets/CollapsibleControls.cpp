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

#include "tomahawk/tomahawkapp.h"
#include "DynamicControlList.h"
#include "DynamicControlWrapper.h"
#include "dynamic/GeneratorInterface.h"
#include "dynamic/DynamicControl.h"

#include <QLabel>
#include <QStackedLayout>
#include <QToolButton>
#include <QAction>
#include <QEasingCurve>
#include <QTimeLine>

using namespace Tomahawk;

CollapsibleControls::CollapsibleControls( QWidget* parent )
    : QWidget( parent )
{
    init();
}

CollapsibleControls::CollapsibleControls( const dynplaylist_ptr& playlist, bool isLocal, QWidget* parent )
    : QWidget( parent )
    , m_dynplaylist( playlist )
{
    init();
    setControls( m_dynplaylist, isLocal );
}

Tomahawk::CollapsibleControls::~CollapsibleControls()
{

}

void 
CollapsibleControls::init()
{
    m_timeline = new QTimeLine( 300, this );
    m_timeline->setUpdateInterval( 8 );
    m_animHeight = -1;
    m_collapseAnimation = false;
    
    connect( m_timeline, SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );
    connect( m_timeline, SIGNAL( finished() ), this, SLOT( onAnimationFinished() ) );
    
    m_layout = new QStackedLayout;
    setContentsMargins( 0, 0, 0, 0 );
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setSpacing( 0 );
    
    m_controls = new Tomahawk::DynamicControlList( this );
    m_layout->addWidget( m_controls );
    connect( m_controls, SIGNAL( toggleCollapse() ), this, SLOT( toggleCollapse() ) );
    
    m_summaryWidget = new QWidget( this );
    // TODO replace
    m_summaryWidget->setMinimumHeight( 24 );
    m_summaryWidget->setMaximumHeight( 24 );
    m_summaryWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    QHBoxLayout* summaryLayout = new QHBoxLayout;
    m_summaryWidget->setLayout( summaryLayout );
    m_summaryWidget->layout()->setMargin( 0 );
    
    m_summary = new QLabel( m_summaryWidget );
    summaryLayout->addWidget( m_summary, 1 );
    m_summaryExpand = new QToolButton( m_summary );
    m_summaryExpand->setIconSize( QSize( 16, 16 ) );
    m_summaryExpand->setIcon( QIcon( RESPATH "images/arrow-down-double.png" ) );
    m_summaryExpand->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_summaryExpand->setAutoRaise( true );
    m_summaryExpand->setContentsMargins( 0, 0, 0, 0 );
    summaryLayout->addWidget( m_summaryExpand );
    m_layout->addWidget( m_summaryWidget );
    connect( m_summaryExpand, SIGNAL( clicked( bool ) ), this, SLOT( toggleCollapse() ) );
    
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
CollapsibleControls::setControls( const dynplaylist_ptr& playlist, bool isLocal )
{
    m_dynplaylist = playlist;
    m_controls->setControls( m_dynplaylist->generator(), m_dynplaylist->generator()->controls(), isLocal );
}

void 
CollapsibleControls::toggleCollapse()
{
    qDebug() << "TOGGLING SIZEHINTS:" << m_controls->height() << m_summaryWidget->sizeHint();
    m_timeline->setEasingCurve( QEasingCurve::OutBack );
    m_timeline->setFrameRange( m_summaryWidget->sizeHint().height(), m_controls->height() );
    m_collapseAnimation = true;
    if( m_layout->currentWidget() == m_controls ) {
        m_summary->setText( m_dynplaylist->generator()->sentenceSummary() );
        m_controls->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
        
        m_timeline->setDirection( QTimeLine::Backward );
        m_timeline->start();
        
    } else {
        m_summaryWidget->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
        m_layout->setCurrentWidget( m_controls );
        
        m_timeline->setDirection( QTimeLine::Forward );
        m_timeline->start();
        
    }
}

void 
CollapsibleControls::onAnimationStep( int step )
{
    qDebug() << "ANIMATION STEP:" << step;
    resize( width(), step );
    m_animHeight = step;
    setMaximumHeight( m_animHeight );
}

void 
CollapsibleControls::onAnimationFinished()
{
    qDebug() << "ANIMATION DONE:" << m_animHeight;
    setMaximumHeight( m_animHeight );
    m_animHeight = -1;
    
    if( m_collapseAnimation && m_layout->currentWidget() == m_controls && m_timeline->direction() == QTimeLine::Backward ) {
        m_layout->setCurrentWidget( m_summaryWidget );
    } else {
        setMaximumHeight( QWIDGETSIZE_MAX );
    }
}

QSize CollapsibleControls::sizeHint() const
{
    if( m_animHeight >= 0 ) {
        return QSize( QWidget::sizeHint().width(), m_animHeight );
    } else {
        return QWidget::sizeHint();
    }
}
