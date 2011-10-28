/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Casey Link <unnamedrambler@gmail.com>
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

#include "BreadcrumbButton.h"

#include "Breadcrumb.h"
#include "combobox.h"
#include "utils/stylehelper.h"
#include "utils/tomahawkutils.h"

#include <QPaintEvent>
#include <QPainter>

using namespace Tomahawk;

BreadcrumbButton::BreadcrumbButton( Breadcrumb* parent, QAbstractItemModel* model )
    : QWidget( parent )
    , m_breadcrumb( parent )
    , m_model( model )
    , m_combo( new ComboBox( this ) )
{
    setFixedHeight( TomahawkUtils::headerHeight() );
    m_combo->setSizeAdjustPolicy( QComboBox::AdjustToContents );

    setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Expanding );

    connect( m_combo, SIGNAL( activated( int ) ), SLOT( comboboxActivated( int ) ) );
}

void
BreadcrumbButton::paintEvent( QPaintEvent* )
{
    QPainter p( this );
    QStyleOption opt;
    opt.initFrom( this );
    QRect r = rect();

    StyleHelper::horizontalHeader( &p, r ); // draw the background

    if( !hasChildren() )
        return;

    bool reverse = opt.direction == Qt::RightToLeft;
    int menuButtonWidth = 12;
    int rightSpacing = 10;
    int left = !reverse ? r.right()-rightSpacing - menuButtonWidth : r.left();
    int right = !reverse ? r.right()-rightSpacing : r.left() + menuButtonWidth;
    int height = r.height();
    QRect arrowRect( ( left + right ) / 2 + ( reverse ? 6 : -6 ), 0, height, height );

    QStyleOption arrowOpt = opt;
    arrowOpt.rect = arrowRect;

    QLine l1( left, 0, right, height/2 );
    QLine l2( left, height, right, height/2 );


    p.setRenderHint( QPainter::Antialiasing, true );

    // Draw the shadow
    QColor shadow( 0, 0, 0, 100 );
    p.translate( 0, -1 );
    p.setPen( shadow );
    p.drawLine( l1 );
    p.drawLine( l2 );

    // Draw the main arrow
    QColor foreGround( "#747474" );
    p.translate( 0, 1 );
    p.setPen( foreGround );
    p.drawLine( l1 );
    p.drawLine( l2 );
}

QSize
BreadcrumbButton::sizeHint() const
{
    // our width = width of combo + 20px for right-arrow and spacing
    const int padding = hasChildren() ? 20 : 5;
    return m_combo->sizeHint() + QSize( padding, 0 );
}


void
BreadcrumbButton::setParentIndex( const QModelIndex& idx )
{
    m_parentIndex = idx;
    // Populate listview with list of children.
    // Then try to find a default
    QStringList list;
    int count = m_model->rowCount( m_parentIndex );
    int defaultIndex = -1, userSelected = -1;
    for ( int i = 0; i < count; ++i )
    {
        QModelIndex idx = m_model->index( i, 0, m_parentIndex );
        if ( idx.isValid() )
        {
            list << idx.data().toString();
            if ( idx.data( Breadcrumb::DefaultRole ).toBool() )
                defaultIndex = i;
            if ( idx.data( Breadcrumb::UserSelectedRole ).toBool() )
                userSelected = i;
        }
    }


    if ( m_combo->count() && list.count() )
    {
        // Check if it's the same, Don't change if it is, as it'll cause flickering
        QStringList old;
        for ( int i = 0; i < m_combo->count(); i++ )
            old << m_combo->itemText( i );

        if ( list == old )
            return;
    }

    m_combo->clear();
    m_combo->addItems( list );

    if ( userSelected > -1 )
        m_combo->setCurrentIndex( userSelected );
    else if ( defaultIndex > -1 )
        m_combo->setCurrentIndex( defaultIndex );

    m_curIndex = m_model->index( m_combo->currentIndex(), 0, m_parentIndex );
    m_combo->adjustSize();
}

void
BreadcrumbButton::comboboxActivated( int idx )
{
    m_model->setData( m_curIndex, false, Breadcrumb::UserSelectedRole );

    QModelIndex selected = m_model->index( idx, 0, m_parentIndex );
    m_curIndex = selected;
    m_model->setData( selected, true, Breadcrumb::UserSelectedRole );

    emit currentIndexChanged( selected );
}


bool
BreadcrumbButton::hasChildren() const
{
    return m_model->rowCount( m_model->index( m_combo->currentIndex(), 0, m_parentIndex ) ) > 0;
}

QModelIndex
BreadcrumbButton::currentIndex() const
{
    return m_model->index( m_combo->currentIndex(), 0, m_parentIndex );
}
