/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "CollapsibleControls.h"

#include "DynamicControlList.h"
#include "DynamicControlWrapper.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/dynamic/DynamicControl.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtils.h"
#include "widgets/ElidedLabel.h"
#include "Source.h"

#include <QLabel>
#include <QStackedLayout>
#include <QToolButton>
#include <QEasingCurve>
#include <QTimeLine>
#include <QPaintEvent>
#include <QPainter>

#include "utils/Logger.h"

using namespace Tomahawk;


CollapsibleControls::CollapsibleControls( QWidget* parent )
    : QWidget( parent )
    , m_isLocal( true )
{
    init();
}


CollapsibleControls::CollapsibleControls( const dynplaylist_ptr& playlist, bool isLocal, QWidget* parent )
    : QWidget( parent )
    , m_dynplaylist( playlist )
    , m_isLocal( isLocal )
{
    init();
    setControls( m_dynplaylist, m_isLocal );
}


Tomahawk::CollapsibleControls::~CollapsibleControls()
{

}


void
CollapsibleControls::init()
{
    m_timeline = new QTimeLine( 250, this );
    m_timeline->setUpdateInterval( 5 );
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
    m_summaryWidget->setMinimumHeight( 24 );
    m_summaryWidget->setMaximumHeight( 24 );
    m_summaryWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_summaryLayout = new QHBoxLayout;
    m_summaryWidget->setLayout( m_summaryLayout );
    m_summaryLayout->setMargin( 0 );
    m_summaryWidget->setContentsMargins( 3, 0, 0, 0 );

    m_summary = new ElidedLabel( m_summaryWidget );
    QFont f = m_summary->font();
    f.setPointSize( f.pointSize() + 1 );
    f.setBold( true );
    m_summary->setFont( f );
    m_summaryLayout->addWidget( m_summary, 1 );
    m_summaryExpand = DynamicControlWrapper::initButton( this );
    m_summaryExpand->setIcon( ImageRegistry::instance()->icon( RESPATH "images/arrow-down-double.svg" ) );
    m_expandL = new QStackedLayout;
    m_expandL->setContentsMargins( 0, 0, 0, 0 );
    m_expandL->setMargin( 0 );
    m_expandL->addWidget( m_summaryExpand );
    m_expandL->addWidget( DynamicControlWrapper::createDummy( m_summaryExpand, this ) );
    m_summaryLayout->addLayout( m_expandL );
    if( m_isLocal )
        m_expandL->setCurrentIndex( 0 );
    else
        m_expandL->setCurrentIndex( 1 );

    m_layout->addWidget( m_summaryWidget );
    connect( m_summaryExpand, SIGNAL( clicked( bool ) ), this, SLOT( toggleCollapse() ) );

    if( m_isLocal )
        m_layout->setCurrentWidget( m_controls );
    else
        m_layout->setCurrentWidget( m_summary );

    connect( m_controls, SIGNAL( controlChanged( Tomahawk::dyncontrol_ptr ) ), SIGNAL( controlChanged( Tomahawk::dyncontrol_ptr ) ) );
    connect( m_controls, SIGNAL( controlsChanged( bool ) ), SIGNAL( controlsChanged( bool ) ) );

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
    m_isLocal = isLocal;
    m_controls->setControls( m_dynplaylist->generator(), m_dynplaylist->generator()->controls() );

    if( !m_isLocal ) {
        m_expandL->setCurrentIndex( 1 );
        m_summary->setText( m_dynplaylist->generator()->sentenceSummary() );
        m_layout->setCurrentWidget( m_summaryWidget );
        setMaximumHeight( m_summaryWidget->sizeHint().height() );
    } else {
        m_expandL->setCurrentIndex( 0  );
    }
}


void
CollapsibleControls::toggleCollapse()
{
//     qDebug() << "TOGGLING SIZEHINTS:" << m_controls->height() << m_summaryWidget->sizeHint();
    m_timeline->setEasingCurve( QEasingCurve::OutBack );
    m_timeline->setFrameRange( m_summaryWidget->sizeHint().height(), m_controls->height() );
    if( m_layout->currentWidget() == m_controls ) {
        m_summary->setText( m_dynplaylist->generator()->sentenceSummary() );
        m_controls->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );

        m_timeline->setDirection( QTimeLine::Backward );
        m_timeline->start();

        m_collapseAnimation = true;
    } else {
        m_summaryWidget->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
        m_layout->setCurrentWidget( m_controls );

        m_timeline->setDirection( QTimeLine::Forward );
        m_timeline->start();

        m_collapseAnimation = false;
    }
}


void
CollapsibleControls::onAnimationStep( int step )
{
//     qDebug() << "ANIMATION STEP:" << step;
    resize( width(), step );
    m_animHeight = step;
    setMaximumHeight( m_animHeight );
}


void
CollapsibleControls::onAnimationFinished()
{
//     qDebug() << "ANIMATION DONE:" << m_animHeight;
    setMaximumHeight( m_animHeight );
    m_animHeight = -1;

    if( m_collapseAnimation ) {
        m_layout->setCurrentWidget( m_summaryWidget );
    } else {
        setMaximumHeight( QWIDGETSIZE_MAX );
    }
}


QSize
CollapsibleControls::sizeHint() const
{
    if( m_animHeight >= 0 ) {
        return QSize( QWidget::sizeHint().width(), m_animHeight );
    } else {
        return QWidget::sizeHint();
    }
}
