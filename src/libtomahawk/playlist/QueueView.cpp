/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "QueueView.h"
#include "ui_QueueView.h"

#include <QVBoxLayout>

#include "widgets/HeaderLabel.h"
#include "playlist/QueueProxyModel.h"
#include "utils/Logger.h"
#include "PlaylistView.h"
#include "Source.h"
#include "utils/TomahawkUtilsGui.h"
#include "widgets/OverlayWidget.h"

using namespace Tomahawk;


QueueView::QueueView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , ui( new Ui::QueueView )
    , m_dragTimer( 0 )
{
    ui->setupUi( this );
    TomahawkUtils::unmarginLayout( layout() );
    setContentsMargins( 0, 0, 0, 0 );

    setHiddenSize( QSize( 0, TomahawkUtils::defaultFontHeight() * 1.4 ) );

    ui->queue->setProxyModel( new QueueProxyModel( ui->queue ) );
    ui->queue->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );

    PlaylistModel* queueModel = new PlaylistModel( this );
    queueModel->setAcceptPlayableQueriesOnly( true );
    ui->queue->proxyModel()->setStyle( PlayableProxyModel::Short );
    queueModel->finishLoading();
    ui->queue->setPlaylistModel( queueModel );
    queueModel->setReadOnly( false );

//    ui->queue->setEmptyTip( tr( "The queue is currently empty. Drop something to enqueue it!" ) );
    ui->queue->setEmptyTip( QString() );

    connect( queueModel, SIGNAL( itemCountChanged( unsigned int ) ), SLOT( updateLabel() ) );
    connect( ui->toggleButton, SIGNAL( clicked() ), SLOT( show() ) );
    connect( this, SIGNAL( animationFinished() ), SLOT( onAnimationFinished() ) );

    ui->toggleButton->installEventFilter( this );
    ui->toggleButton->setCursor( Qt::PointingHandCursor );
}


QueueView::~QueueView()
{
    qDebug() << Q_FUNC_INFO;
}


void
QueueView::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


PlaylistView*
QueueView::queue() const
{
    return ui->queue;
}


bool
QueueView::eventFilter( QObject* obj, QEvent* ev )
{
    if ( obj == ui->toggleButton )
    {
        if ( ev->type() == QEvent::DragEnter )
        {
            ev->accept();

            if ( m_dragTimer == 0 )
            {
                m_dragTimer = new QTimer( this );
                m_dragTimer->setInterval( 1000 );
                m_dragTimer->setSingleShot( true );
                connect( m_dragTimer, SIGNAL( timeout() ), this, SLOT( show() ) );
                m_dragTimer->start();
            }
        }
        else if ( ev->type() == QEvent::DragLeave || ev->type() == QEvent::Drop )
        {
            delete m_dragTimer;
            m_dragTimer = 0;
        }
    }

    return QObject::eventFilter( obj, ev );
}


void
QueueView::hide()
{
    disconnect( ui->toggleButton, SIGNAL( clicked() ), this, SLOT( hide() ) );
    connect( ui->toggleButton, SIGNAL( clicked() ), SLOT( show() ) );
    disconnect( ui->toggleButton, SIGNAL( resized( QPoint ) ), this, SIGNAL( resizeBy( QPoint ) ) );

    ui->queue->overlay()->setVisible( false );
    AnimatedWidget::hide();
    updateLabel();
}


void
QueueView::show()
{
    disconnect( ui->toggleButton, SIGNAL( clicked() ), this, SLOT( show() ) );
    connect( ui->toggleButton, SIGNAL( clicked() ), SLOT( hide() ) );
    connect( ui->toggleButton, SIGNAL( resized( QPoint ) ), SIGNAL( resizeBy( QPoint ) ) );

    ui->queue->overlay()->setVisible( false );
    AnimatedWidget::show();
    updateLabel();
}


void
QueueView::onShown( QWidget* widget, bool animated )
{
    if ( widget != this )
        return;

    AnimatedWidget::onShown( widget, animated );
}


void
QueueView::onHidden( QWidget* widget, bool animated )
{
    if ( widget != this )
        return;

    AnimatedWidget::onHidden( widget, animated );
}


void
QueueView::onAnimationFinished()
{
    ui->queue->overlay()->setVisible( true );
}


void
QueueView::updateLabel()
{
    if ( isHidden() )
    {
        const unsigned int c = queue()->model()->rowCount( QModelIndex() );

        if ( c )
            ui->toggleButton->setText( tr( "Open Queue - %n item(s)", "", c ) );
        else
            ui->toggleButton->setText( tr( "Open Queue" ) );
    }
    else
    {
        ui->toggleButton->setText( tr( "Close Queue" ) );
    }
}
