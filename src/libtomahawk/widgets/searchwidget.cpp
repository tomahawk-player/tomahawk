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

#include "searchwidget.h"
#include "ui_searchwidget.h"

#include <QPushButton>
#include <QDialogButtonBox>

#include "sourcelist.h"
#include "viewmanager.h"
#include "playlist/playlistmodel.h"
#include "widgets/overlaywidget.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"


SearchWidget::SearchWidget( const QString& search, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SearchWidget )
    , m_search( search )
{
    ui->setupUi( this );

    m_resultsModel = new PlaylistModel( ui->resultsView );
    ui->resultsView->setPlaylistModel( m_resultsModel );
    ui->resultsView->overlay()->setEnabled( false );
    ui->resultsView->sortByColumn( PlaylistModel::Score, Qt::DescendingOrder );

    TomahawkUtils::unmarginLayout( ui->verticalLayout );
    ui->resultsView->setContentsMargins( 0, 0, 0, 0 );
    ui->resultsView->setFrameShape( QFrame::NoFrame );
    ui->resultsView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    m_queries << Tomahawk::Query::get( search, uuid() );

    foreach ( const Tomahawk::query_ptr& query, m_queries )
    {
        connect( query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ), SLOT( onResultsFound( QList<Tomahawk::result_ptr> ) ) );
    }
}


SearchWidget::~SearchWidget()
{
    delete ui;
}


void
SearchWidget::changeEvent( QEvent* e )
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


void
SearchWidget::onResultsFound( const QList<Tomahawk::result_ptr>& results )
{
    foreach( const Tomahawk::result_ptr& result, results )
    {
        if ( !result->collection().isNull() && !result->isOnline() )
            continue;

        QList< Tomahawk::result_ptr > rl;
        rl << result;

        Tomahawk::query_ptr q = result->toQuery();
        q->addResults( rl );
        qDebug() << result->toString();

        m_resultsModel->append( q );
    }
}
