/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Casey Link <unnamedrambler@gmail.com>
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

#include "siblingcrumbbutton.h"

#include "combobox.h"
#include "utils/stylehelper.h"

#include <QTimer>
#include <QDebug>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

BreadcrumbButtonBase* SiblingCrumbButtonFactory::newButton(QModelIndex index, BreadcrumbBar *parent)
{
    return new SiblingCrumbButton(index, parent);
}

SiblingCrumbButton::SiblingCrumbButton(
    QModelIndex index, BreadcrumbBar *parent)
    : BreadcrumbButtonBase(parent),
      m_index(index), m_combo( new ComboBox(this) )
{
    m_combo->setSizeAdjustPolicy( QComboBox::AdjustToContents );
    setIndex(index);
    connect(m_combo, SIGNAL(activated(int)), SLOT(comboboxActivated(int)));
}

void SiblingCrumbButton::setIndex( QModelIndex index )
{
    if ( m_index == index && text() == index.data().toString() )
        return;
    m_index = index;
    setText( index.data().toString() );
    fillCombo();
}

QModelIndex SiblingCrumbButton::index() const
{
    return m_index;
}

void SiblingCrumbButton::setActive( bool active )
{
    Q_UNUSED( active );
}

bool SiblingCrumbButton::isActive() const
{
    return false;
}

QSize SiblingCrumbButton::sizeHint() const
{
    // our width = width of combo + 20px for right-arrow and spacing
    const int padding = hasChildren() ? 20 : 5;
    return m_combo->sizeHint() + QSize( padding, 0 );
}

void SiblingCrumbButton::paintEvent( QPaintEvent *event )
{
    Q_UNUSED( event );

    QPainter p( this );
    QStyleOption opt;
    opt.initFrom( this );
    QRect r = opt.rect;

    StyleHelper::horizontalHeader( &p, r ); // draw the background

    if( !hasChildren() )
        return;

    bool reverse = opt.direction == Qt::RightToLeft;
    int menuButtonWidth = 12;
    int rightSpacing = 10;
    int left = !reverse ? r.right()-rightSpacing - menuButtonWidth : r.left();
    int right = !reverse ? r.right()-rightSpacing : r.left() + menuButtonWidth;
    int height = sizeHint().height();
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

void SiblingCrumbButton::fillCombo()
{
    QStringList list;
    int count = breadcrumbBar()->model()->rowCount(m_index.parent());
    for(int i = 0; i < count; ++i) {
        QModelIndex sibling = m_index.sibling(i,0);
        if( sibling.isValid() )
            list << sibling.data().toString();
    }

    m_combo->clear();
    m_combo->addItems(list);
    m_combo->setCurrentIndex( m_combo->findText(text()));
    m_combo->adjustSize();
}

void SiblingCrumbButton::comboboxActivated(int i)
{
    QModelIndex activated = m_index.sibling(i,0);
    int count = breadcrumbBar()->model()->rowCount(activated);
    if( count > 0 ) {
        qDebug() << "activated" << activated.child(0,0).data().toString();
        breadcrumbBar()->currentChangedTriggered(activated.child(0,0));
    } else {
        // if it has no children, then emit itself
        breadcrumbBar()->currentChangedTriggered(activated);
    }
}

void SiblingCrumbButton::activateSelf()
{
    comboboxActivated(m_index.row());
}

bool SiblingCrumbButton::hasChildren() const
{
    return m_index.model()->hasChildren(m_index);
}
