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

#include "DynamicWidget.h"

#include <QVBoxLayout>
#include <QLabel>

#include "DynamicControlList.h"
#include "playlistview.h"
#include "playlistmodel.h"
#include "trackproxymodel.h"
#include "dynamic/GeneratorInterface.h"

using namespace Tomahawk;

DynamicWidget::DynamicWidget( const Tomahawk::dynplaylist_ptr& playlist, QWidget* parent )
    : QWidget(parent)
    , m_header( 0 )
    , m_playlist( playlist )
    , m_controls( 0 )
    , m_splitter( 0 )
    , m_view( 0 )
    , m_model()
{
    setLayout( new QVBoxLayout );
    
    m_header = new QLabel( "TODO DYN PLAYLIST HEADER", this );
    layout()->addWidget( m_header );
    
    m_splitter = new AnimatedSplitter( this );
    m_splitter->setOrientation( Qt::Vertical );
    
    m_controls = new DynamicControlList( m_splitter );
    m_model = new PlaylistModel( this );
    m_view = new PlaylistView( this );
    m_view->setModel( m_model );
    
    m_splitter->addWidget( m_controls );
    m_splitter->addWidget( m_view );
    m_splitter->setGreedyWidget( 1 );
    
    if( !m_playlist.isNull() ) {
        m_controls->setControls( m_playlist->generator()->controls() );
        
        m_model->loadPlaylist( m_playlist );
    }
    
}

DynamicWidget::~DynamicWidget()
{

}

void 
DynamicWidget::setPlaylist(const Tomahawk::dynplaylist_ptr& playlist)
{
    
}

PlaylistInterface* 
DynamicWidget::playlistInterface() const
{
    return m_view->proxyModel();
}
