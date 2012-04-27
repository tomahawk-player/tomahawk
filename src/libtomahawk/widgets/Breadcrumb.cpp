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

#include "Breadcrumb.h"

#include "BreadcrumbButton.h"
#include "utils/StyleHelper.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtilsGui.h"

#include <QStylePainter>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QDebug>

using namespace Tomahawk;

Breadcrumb::Breadcrumb( QWidget* parent, Qt::WindowFlags f )
    : QWidget( parent, f )
    , m_model( 0 )
    , m_buttonlayout( new QHBoxLayout( this ) )
{
    TomahawkUtils::unmarginLayout( m_buttonlayout );
    m_buttonlayout->setAlignment( Qt::AlignLeft );

    setAutoFillBackground( true );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    setLayoutDirection( Qt::LeftToRight );
    setLayout( m_buttonlayout );
    show();
}


Breadcrumb::~Breadcrumb()
{
}


void
Breadcrumb::setModel( QAbstractItemModel* model )
{
    foreach ( BreadcrumbButton* b, m_buttons )
        b->deleteLater();;
    m_buttons.clear();

    m_model = model;
    updateButtons( QModelIndex() );
}


void
Breadcrumb::setRootIcon( const QPixmap& pm )
{
    m_rootIcon = pm;

    QPushButton* button = new QPushButton( QIcon( m_rootIcon ), "", this );
    button->setFlat( true );
    button->setStyleSheet( "QPushButton{ background-color: transparent; border: none; width:16px; height:16px;}" );
    m_buttonlayout->insertWidget( 0, button );
    m_buttonlayout->insertSpacing( 0,5 );
    m_buttonlayout->insertSpacing( 2,5 );
}


void
Breadcrumb::paintEvent( QPaintEvent* )
{
    QStylePainter p( this );
    StyleHelper::horizontalHeader( &p, rect() );
}


// updateFrom is the item that has changed---all children must be recomputed
// if invalid, redo the whole breadcrumb
void
Breadcrumb::updateButtons( const QModelIndex& updateFrom )
{
//     qDebug() << "Updating buttons:" << updateFrom.data();
    int cur = 0;
    QModelIndex idx = updateFrom;
    for ( int i = 0; i < m_buttons.count(); i++ )
    {
//         qDebug() << "Checking if this breadcrumb item changed:" << m_buttons[ i ]->currentIndex().data() << updateFrom.data() << ( m_buttons[ i ]->currentIndex() != updateFrom);
        if ( m_buttons[ i ]->currentIndex() == updateFrom )
        {
            cur = i;
            break;
        }
    }

    // We set the parent index, so go up one
    idx = idx.parent();

    // Ok, changed all indices that are at cur or past it. lets update them
    // When we get to the "end" of the tree, the leaf node is the chart itself
//     qDebug() << "DONE and beginning iteration:" << idx.data();
    while ( m_model->rowCount( idx ) > 0 )
    {
//         qDebug() << "CHANGED AND iterating:" << idx.data();
        BreadcrumbButton* btn = 0;
        if ( m_buttons.size() <= cur )
        {
            // We have to create a new button, doesn't exist yet
            btn = new BreadcrumbButton( this, m_model );
            connect( btn, SIGNAL( currentIndexChanged( QModelIndex ) ), this, SLOT( breadcrumbComboChanged( QModelIndex ) ) );

            m_buttonlayout->addWidget( btn );
            btn->show();

            // Animate all buttons except the first
            if ( m_buttons.count() > 0 && isVisible() )
            {
                QPropertyAnimation* animation = new QPropertyAnimation( btn, "pos" );
                animation->setDuration( 300 );
                animation->setStartValue( m_buttons.last()->pos() );
                animation->setEndValue( btn->pos() );
                animation->start( QAbstractAnimation::DeleteWhenStopped );
            }

            m_buttons.append( btn );
        }
        else
        {
            // Got a button already, we just want to change the contents
            btn = m_buttons[ cur ];
        }

        // The children of idx are what populates this combobox.
        // It takes care of setting the default/user-populated value.
        btn->setParentIndex( idx );

        // Repeat with children
        idx = btn->currentIndex();

        cur++;
    }

    // extra buttons to delete! (cur is 0-indexed)
    while ( m_buttons.size() > cur )
    {
        BreadcrumbButton* b = m_buttons.takeLast();
        m_buttonlayout->removeWidget( b );
        b->deleteLater();
    }

    // Now we're at the leaf, lets activate the chart
    emit activateIndex( idx );
}


void
Breadcrumb::breadcrumbComboChanged( const QModelIndex& childSelected )
{
    // Some breadcrumb buttons' combobox changed. lets update the child breadcrumbs
    tDebug() << "Combo changed:" << childSelected.data();
    updateButtons( childSelected );
}
