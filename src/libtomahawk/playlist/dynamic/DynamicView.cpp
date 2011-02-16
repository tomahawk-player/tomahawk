/****************************************************************************************
 * Copyright (c) 2011 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "DynamicView.h"

#include "widgets/overlaywidget.h"
#include "playlistmodel.h"
#include "trackproxymodel.h"
#include "trackheader.h"

#include <QPainter>
#include <QPaintEvent>
#include <QtGui/qpaintengine.h>
#include <QScrollBar>

#include "DynamicModel.h"
#include <QApplication>

using namespace Tomahawk;

#define FADE_LENGTH 800
#define SLIDE_LENGTH 300
#define SLIDE_OFFSET 500
#define LONG_MULT 0 // to avoid superfast slides when the length is long, make it longer incrementally

DynamicView::DynamicView( QWidget* parent )
        : PlaylistView( parent )
        , m_onDemand( false )
        , m_checkOnCollapse( false )
        , m_fadebg( false )
{
    m_fadeOutAnim.setDuration( FADE_LENGTH );
    m_fadeOutAnim.setCurveShape( QTimeLine::LinearCurve );
    m_fadeOutAnim.setFrameRange( 100, 0 );
    m_fadeOutAnim.setUpdateInterval( 5 );
    
    QEasingCurve curve( QEasingCurve::OutBounce );
    curve.setAmplitude( .25 );
    m_slideAnim.setEasingCurve( curve );
    m_slideAnim.setDirection( QTimeLine::Forward );
    m_fadeOutAnim.setUpdateInterval( 5 );
    
    
    connect( &m_fadeOutAnim, SIGNAL( frameChanged( int ) ), viewport(), SLOT( update() ) );
    connect( &m_fadeOutAnim, SIGNAL( finished() ), this, SLOT( animFinished() ) );
}

DynamicView::~DynamicView()
{

}

void 
DynamicView::setModel( DynamicModel* model)
{
    m_model = model;
    PlaylistView::setModel( model );
    
    connect( model, SIGNAL( trackCountChanged( unsigned int ) ), SLOT( onTrackCountChanged( unsigned int ) ) );
    connect( model, SIGNAL( checkForOverflow() ), this, SLOT( checkForOverflow() ) );
}

void
DynamicView::setOnDemand( bool onDemand )
{
    m_onDemand = onDemand;
    
    if( m_onDemand )
        setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    else
        setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
}

void 
DynamicView::setReadOnly( bool readOnly )
{
    m_readOnly = readOnly;
}

void 
DynamicView::showMessageTimeout( const QString& title, const QString& body )
{
    m_title = title;
    m_body = body;
    
    overlay()->setText( QString( "%1:\n\n%2" ).arg( m_title, m_body ) );
    overlay()->show( 10 );
}

void 
DynamicView::showMessage(const QString& message)
{
    overlay()->setText( message );
    overlay()->show();
}


void 
DynamicView::onTrackCountChanged( unsigned int tracks )
{
    if ( tracks == 0 )
    {
        if( m_onDemand ) {
            if( m_readOnly )
                overlay()->setText( tr( "Press play to begin listening to this custom station!" ) );
            else
                overlay()->setText( tr( "Add some filters above, and press play to begin listening to this custom station!" ) );
        } else
            if( m_readOnly )
                overlay()->setText( tr( "Press Generate to get started!" ) );
            else
                overlay()->setText( tr( "Add some filters above, and press Generate to get started!" ) );
        if( !overlay()->shown() )
            overlay()->show();
    }
    else
        overlay()->hide();
}

void
DynamicView::checkForOverflow()
{
    if( !m_onDemand || proxyModel()->rowCount( QModelIndex() ) == 0 )
        return;

    if( m_fadeOutAnim.state() == QTimeLine::Running )
        m_checkOnCollapse = true;

    /// We don't want stations to grow forever, because we don't want the view to have to scroll
    /// So if there are too many tracks, we remove some that have already been played
    /// Our threshold is 4 rows to the end. That's when we collapse.
    QModelIndex last = proxyModel()->index( proxyModel()->rowCount( QModelIndex() ) - 1, 0, QModelIndex() );
    QRect lastRect = visualRect( last );
    qDebug() << "Checking viewport height of" << viewport()->height() << "and last track bottom:" << lastRect.bottomLeft().y() << "under threshold" << 4 * lastRect.height();
    if( viewport()->height() - lastRect.bottomLeft().y() <= ( 4 * lastRect.height() ) ) {
        qDebug() << "Deciding to remove some tracks from this station";

        // figure out how many to remove. lets get rid of 1/3rd of the backlog, visually.
        int toRemove = ( viewport()->height() / 3 ) / lastRect.height();
        qDebug() << "Decided to remove" << toRemove << "rows!";
        collapseEntries( 0, toRemove, proxyModel()->rowCount( QModelIndex() ) - toRemove );
    }
}

void 
DynamicView::collapseEntries( int startRow, int num, int numToKeep )
{
    qDebug() << "BEGINNING TO COLLAPSE FROM" << startRow << num << numToKeep;
    if( m_fadeOutAnim.state() == QTimeLine::Running ) {
        qDebug() << "COLLAPSING TWICE, aborting!";
        return;
    }

    /// Two options: Either we are overflowing our view, or we're not. If we are, it's because the search for a playable track
    /// went past the limit of the view. Just fade out from the beginning to the end in that case. otherwise, animate a slide
    bool justFade = false;
    int realNum = num;
    QModelIndex last = indexAt( QPoint( 3, viewport()->height() - 3 ) );
    if( last.isValid() && last.row() < startRow + num ) {
        justFade = true;
        realNum = last.row();
    }
     // we capture the image of the rows we're going to collapse
    // then we capture the image of the target row we're going to animate downwards
    // then we fade the first image out while sliding the second image up.
    QModelIndex topLeft = proxyModel()->index( startRow, 0, QModelIndex() );
    QModelIndex bottomRight = proxyModel()->index( startRow + realNum - 1, proxyModel()->columnCount( QModelIndex() ) - 1, QModelIndex() );
    QItemSelection sel( topLeft, bottomRight );
    QRect fadingRect = visualRegionForSelection( sel ).boundingRect();
    QRect fadingRectViewport = fadingRect; // all values that we use in paintEvent() have to be in viewport coords
    fadingRect.moveTo( viewport()->mapTo( this, fadingRect.topLeft() ) );

    m_fadingIndexes = QPixmap::grabWidget( this, fadingRect ); // but all values we use to grab the widgetr have to be in scrollarea coords :(
    m_fadingPointAnchor = QPoint( 0, fadingRectViewport.topLeft().y() );
    
    // get the background
    m_bg = QPixmap( m_fadingIndexes.size() );
    m_bg.fill( Qt::white );
    QPainter p( &m_bg );
    QStyleOptionViewItemV4 opt = viewOptions();
    // code taken from QTreeViewPrivate::paintAlternatingRowColors
    if( alternatingRowColors() ) {
        m_fadebg = !style()->styleHint( QStyle::SH_ItemView_PaintAlternatingRowColorsForEmptyArea, &opt );
        qDebug() << "PAINTING ALTERNATING ROW BG!: " << fadingRectViewport;
        int rowHeight = itemDelegate()->sizeHint( opt, QModelIndex() ).height();
        int y = m_fadingIndexes.rect().topLeft().y();
        int current = startRow;
        while( y <= m_fadingIndexes.rect().bottomLeft().y() ) {
            opt.rect.setRect(0, y, viewport()->width(), rowHeight);
            qDebug() << "Painting a row from " << y << "to" << y + rowHeight << ":" << opt.rect;
            if( current & 1 ) {
                opt.features |= QStyleOptionViewItemV2::Alternate;
            } else {
                opt.features &= ~QStyleOptionViewItemV2::Alternate;
            }
            ++current;
            style()->drawPrimitive( QStyle::PE_PanelItemViewRow, &opt, &p );
            y += rowHeight;
        }
    }
    m_fadeOutAnim.start();
    
    qDebug() << "Grabbed fading indexes from rect:" << fadingRect << m_fadingIndexes.size();
    
    if( !justFade ) {
    /// sanity checks. make sure we have all the rows we need
        int firstSlider = startRow + realNum;
        Q_ASSERT( firstSlider + numToKeep - 1 <= proxyModel()->rowCount() );
     
        topLeft = proxyModel()->index( startRow + realNum, 0, QModelIndex() );
        bottomRight = proxyModel()->index( startRow + realNum + numToKeep - 1, proxyModel()->columnCount( QModelIndex() ) - 1, QModelIndex() );
        QRect slidingRect = visualRegionForSelection( QItemSelection( topLeft, bottomRight ) ).boundingRect();
        QRect slidingRectViewport = slidingRect;
        // map internal view coord to external qscrollarea
        slidingRect.moveTo( viewport()->mapTo( this, slidingRect.topLeft() ) );
    
        m_slidingIndex = QPixmap::grabWidget( this, slidingRect );
        m_bottomAnchor = QPoint( 0, slidingRectViewport.topLeft().y() );
        m_bottomOfAnim = QPoint( 0, slidingRectViewport.bottomLeft().y() );
        qDebug() << "Grabbed sliding index from rect:" << slidingRect << m_slidingIndex.size();
    
        // slide from the current position to the new one
        int frameRange = fadingRect.topLeft().y() - slidingRect.topLeft().y();
        m_slideAnim.setDuration( SLIDE_LENGTH + frameRange * LONG_MULT );
        m_slideAnim.setFrameRange( slidingRectViewport.topLeft().y(), fadingRectViewport.topLeft().y() );
        
        QTimer::singleShot( SLIDE_OFFSET, &m_slideAnim, SLOT( start() ) );
    }
    
    // delete the actual indices
    QModelIndexList todel;
    for( int i = 0; i < num; i++ ) {
        for( int k = 0; k < proxyModel()->columnCount( QModelIndex() ); k++ ) {
            todel << proxyModel()->index( startRow + i, k );
        }
    }
    proxyModel()->removeIndexes( todel );
}

void
DynamicView::animFinished()
{
    if( m_checkOnCollapse )
        checkForOverflow();
    m_checkOnCollapse = false;
}

void 
DynamicView::paintEvent( QPaintEvent* event )
{
    TrackView::paintEvent(event);
    
    QPainter p( viewport() );
    if( m_fadeOutAnim.state() == QTimeLine::Running ) { // both run together
        p.save();
        QRect bg = m_fadingIndexes.rect();
        bg.moveTo( m_fadingPointAnchor ); // cover up the background
        if( m_fadebg ) {
            p.save();
            p.fillRect( bg, Qt::white );
            
            p.setOpacity( 1 - m_fadeOutAnim.currentValue() );
        }
        p.drawPixmap( bg, m_bg );
        if( m_fadebg ) {
            p.restore();
        }
        
//         qDebug() << "FAST SETOPACITY:" << p.paintEngine()->hasFeature(QPaintEngine::ConstantOpacity);
        p.setOpacity( 1 - m_fadeOutAnim.currentValue() );
        p.drawPixmap( m_fadingPointAnchor, m_fadingIndexes );
        
        p.restore();   
        
        if( m_slideAnim.state() == QTimeLine::Running ) {
            // draw the collapsing entry
            QRect bg = m_slidingIndex.rect();
            bg.moveTo( m_bottomAnchor );
            bg.setBottom( m_bottomOfAnim.y() );
            p.fillRect( bg, Qt::white );
            p.drawPixmap( 0, m_slideAnim.currentFrame(), m_slidingIndex );
        } else if( m_fadeOutAnim.state() == QTimeLine::Running ) {
            p.drawPixmap( 0, m_bottomAnchor.y(), m_slidingIndex );
        }
    }
}

