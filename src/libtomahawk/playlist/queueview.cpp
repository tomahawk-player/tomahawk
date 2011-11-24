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

#include "queueview.h"
#include "ui_queueview.h"

#include <QVBoxLayout>

#include "widgets/HeaderLabel.h"
#include "playlist/queueproxymodel.h"
#include "widgets/overlaywidget.h"
#include "utils/logger.h"
#include "playlistview.h"

using namespace Tomahawk;


QueueView::QueueView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , ui( new Ui::QueueView )
    , m_dragTimer( 0 )
{
    ui->setupUi( this );
    TomahawkUtils::unmarginLayout( layout() );
    setContentsMargins( 0, 0, 0, 0 );

    setHiddenSize( QSize( 0, 22 ) );

    ui->queue->setProxyModel( new QueueProxyModel( ui->queue ) );
    ui->queue->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    ui->queue->setFrameShape( QFrame::NoFrame );
    ui->queue->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->queue->overlay()->setEnabled( false );

    connect( ui->toggleButton, SIGNAL( clicked() ), SLOT( show() ) );

    ui->toggleButton->installEventFilter( this );
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
    ui->toggleButton->setText( tr( "Show Queue" ) );
    emit hideWidget();
}


void
QueueView::show()
{
    disconnect( ui->toggleButton, SIGNAL( clicked() ), this, SLOT( show() ) );
    connect( ui->toggleButton, SIGNAL( clicked() ), SLOT( hide() ) );
    ui->toggleButton->setText( tr( "Hide Queue" ) );
    emit showWidget();
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
