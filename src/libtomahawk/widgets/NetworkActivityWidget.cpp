/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "NetworkActivityWidget.h"
#include "ui_NetworkActivityWidget.h"

#include "utils/AnimatedSpinner.h"
#include "utils/TomahawkUtilsGui.h"

#include <QtConcurrentRun>

NetworkActivityWidget::NetworkActivityWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::NetworkActivityWidget )
    //, m_sortedProxy( 0 )
    //, m_loading( true )
{
    ui->setupUi( this );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->stackLeft->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->breadCrumbLeft->layout() );

//    m_crumbModelLeft = new QStandardItemModel( this );
//    m_sortedProxy = new QSortFilterProxyModel( this );
//    m_sortedProxy->setDynamicSortFilter( true );
//    m_sortedProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );

    ui->breadCrumbLeft->setRootIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::NetworkActivity, TomahawkUtils::Original ) );
//    connect( ui->breadCrumbLeft, SIGNAL( activateIndex( QModelIndex ) ), SLOT( leftCrumbIndexChanged( QModelIndex ) ) );

    ui->tracksViewLeft->setHeaderHidden( true );
    ui->tracksViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
//    PlaylistChartItemDelegate* del = new PlaylistChartItemDelegate( ui->tracksViewLeft, ui->tracksViewLeft->proxyModel() );
//    ui->tracksViewLeft->setItemDelegate( del );
    ui->tracksViewLeft->setUniformRowHeights( false );

    m_playlistInterface = ui->tracksViewLeft->playlistInterface();

    // Lets have a spinner until loaded
    ui->breadCrumbLeft->setVisible( false );
    m_spinner = QSharedPointer<AnimatedSpinner>( new AnimatedSpinner( ui->tracksViewLeft ) );
    m_spinner->fadeIn();
}

NetworkActivityWidget::~NetworkActivityWidget()
{
    delete ui;
}

Tomahawk::playlistinterface_ptr
NetworkActivityWidget::playlistInterface() const
{
    return m_playlistInterface;
}

bool
NetworkActivityWidget::jumpToCurrentTrack()
{
    // TODO
    return false;
}

void
NetworkActivityWidget::fetchData()
{
    // Do not block the UI thread
    QtConcurrent::run( this, &NetworkActivityWidget::actualFetchData );
}

void
NetworkActivityWidget::actualFetchData()
{
}
