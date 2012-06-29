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

#include "NewPlaylistWidget.h"
#include "ui_NewPlaylistWidget.h"

#include <QPushButton>
#include <QDialogButtonBox>

#include "SourceList.h"
#include "ViewManager.h"
#include "playlist/PlaylistModel.h"
#include "widgets/OverlayWidget.h"

#include "utils/XspfLoader.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#define FILTER_TIMEOUT 280


NewPlaylistWidget::NewPlaylistWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::NewPlaylistWidget )
{
    ui->setupUi( this );

    m_saveButton = new QPushButton( tr( "&Create Playlist" ) );
    m_saveButton->setDefault( true );
    m_saveButton->setEnabled( false );

    ui->buttonBox->addButton( m_saveButton, QDialogButtonBox::AcceptRole );

    connect( ui->titleEdit, SIGNAL( textChanged( QString ) ), SLOT( onTitleChanged( QString ) ) );
    connect( ui->tagEdit, SIGNAL( textChanged( QString ) ), SLOT( onTagChanged() ) );
    connect( ui->buttonBox, SIGNAL( accepted() ), SLOT( savePlaylist() ) );
    connect( ui->buttonBox, SIGNAL( rejected() ), SLOT( cancel() ) );

    m_suggestionsModel = new PlaylistModel( ui->suggestionsView );
    ui->suggestionsView->setPlaylistModel( m_suggestionsModel );
    ui->suggestionsView->overlay()->setEnabled( false );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( updateSuggestions() ) );

    ui->titleEdit->setFocus();
}


NewPlaylistWidget::~NewPlaylistWidget()
{
    delete ui;
}


void
NewPlaylistWidget::changeEvent( QEvent* e )
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
NewPlaylistWidget::onTitleChanged( const QString& title )
{
    m_saveButton->setEnabled( !title.isEmpty() );
}


void
NewPlaylistWidget::onTagChanged()
{
    m_tag = ui->tagEdit->text();

    m_filterTimer.stop();
    m_filterTimer.setInterval( FILTER_TIMEOUT );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}


void
NewPlaylistWidget::updateSuggestions()
{
    QUrl url( QString( "http://ws.audioscrobbler.com/1.0/tag/%1/toptracks.xspf" ).arg( m_tag ) );

    XSPFLoader* loader = new XSPFLoader( false );
    connect( loader, SIGNAL( ok( Tomahawk::playlist_ptr ) ), SLOT( suggestionsFound() ) );

    loader->load( url );
}


void
NewPlaylistWidget::suggestionsFound()
{
    XSPFLoader* loader = qobject_cast<XSPFLoader*>( sender() );

    m_queries = loader->entries();

    delete m_suggestionsModel;
    m_suggestionsModel = new PlaylistModel( ui->suggestionsView );
    ui->suggestionsView->setPlaylistModel( m_suggestionsModel );

    m_suggestionsModel->appendQueries( m_queries );

    loader->deleteLater();
}


void
NewPlaylistWidget::savePlaylist()
{
    Tomahawk::playlist_ptr playlist;

    playlist = Tomahawk::Playlist::create( SourceList::instance()->getLocal(), uuid(), ui->titleEdit->text(), "", "", false, m_queries );

    ViewManager::instance()->show( playlist );
    cancel();
}


void
NewPlaylistWidget::cancel()
{
    // will be deleted by viewmanager
    emit destroyed( this );
}
