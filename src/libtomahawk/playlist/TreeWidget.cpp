/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "TreeWidget.h"

#include "collection/Collection.h"
#include "utils/TomahawkUtilsGui.h"

#include <QBoxLayout>

TreeWidget::TreeWidget( QWidget* parent )
    : QWidget( parent )
    , m_view( new TreeView( this ) )
    , m_header( new ScriptCollectionHeader( this ) )
{
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget( m_header );
    mainLayout->addWidget( m_view );
    setLayout( mainLayout );
    TomahawkUtils::unmarginLayout( mainLayout );

    connect( m_header, SIGNAL( filterTextChanged( QString ) ),
             this, SLOT( setFilter( QString ) ) );
    connect( m_view, SIGNAL( modelChanged() ),
             this, SLOT( onModelChanged() ) );
    connect( m_header, SIGNAL( refreshClicked() ),
             this, SLOT( onRefreshClicked() ) );
}


TreeWidget::~TreeWidget()
{}


TreeView*
TreeWidget::view() const
{
    return m_view;
}


QWidget*
TreeWidget::widget()
{
    return this;
}


Tomahawk::playlistinterface_ptr
TreeWidget::playlistInterface() const
{
    return m_view->proxyModel()->playlistInterface();
}


QString
TreeWidget::title() const
{
    return m_view->model()->title();
}


QString
TreeWidget::description() const
{
    return m_view->model()->description();
}


QPixmap
TreeWidget::pixmap() const
{
    return m_view->model()->icon();
}


bool
TreeWidget::showFilter() const
{
     return true;
}


bool
TreeWidget::setFilter( const QString& filter )
{
    ViewPage::setFilter( filter );
    m_view->proxyModel()->setFilter( filter );
    return true;
}


void
TreeWidget::onModelChanged()
{
    m_header->setCaption( m_view->model()->title() );
    m_header->setDescription( m_view->model()->description() );
    m_header->setPixmap( m_view->model()->icon() );
    if ( !m_view->model()->collection().isNull() )
        m_header->setRefreshVisible( m_view->model()->collection()->backendType() != Tomahawk::Collection::DatabaseCollectionType );
}


void
TreeWidget::onRefreshClicked()
{
    if ( m_view->model() && !m_view->model()->collection().isNull() )
        m_view->model()->reloadCollection();
}


bool
TreeWidget::jumpToCurrentTrack()
{
    return m_view->jumpToCurrentTrack();
}


bool
TreeWidget::showInfoBar() const
{
    return false;
}
