/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2014,      Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "LoadPlaylistDialog.h"
#include "ui_LoadPlaylistDialog.h"

#include "TomahawkSettings.h"
#include "Source.h"

#include <QFileDialog>

LoadPlaylistDialog::LoadPlaylistDialog( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
    , m_ui( new Ui_LoadPlaylist )
{
    m_ui->setupUi( this );

#ifdef Q_OS_MAC
    m_ui->horizontalLayout->setContentsMargins( 0, 0, 0, 0 );
    m_ui->horizontalLayout->setSpacing( 5 );
#endif

    setMinimumSize( sizeHint() );
    m_ui->autoUpdate->setEnabled( false );

    connect( m_ui->navigateButton, SIGNAL( clicked( bool ) ), this, SLOT( getLocalFile() ) );
    connect( m_ui->lineEdit, SIGNAL( textChanged( QString ) ), this, SLOT( onUrlChanged() ) );
}


LoadPlaylistDialog::~LoadPlaylistDialog()
{
}


void
LoadPlaylistDialog::getLocalFile()
{
    const QString path = TomahawkSettings::instance()->importPlaylistPath();
    QString url = QFileDialog::getOpenFileName( this, tr( "Load Playlist" ), path, tr( "Playlists (*.xspf *.m3u *.jspf)" ) );

    if ( !url.isEmpty() )
    {
        const QFileInfo fi( url );
        TomahawkSettings::instance()->setImportPlaylistPath( fi.absoluteDir().absolutePath() );
    }

    m_ui->lineEdit->setText( url );
}


void
LoadPlaylistDialog::onUrlChanged()
{
    m_ui->autoUpdate->setEnabled( m_ui->lineEdit->text().trimmed().startsWith( "http://" ) );
}


QString
LoadPlaylistDialog::url() const
{
    return m_ui->lineEdit->text();
}


bool
LoadPlaylistDialog::autoUpdate() const
{
    return m_ui->autoUpdate->isChecked();
}
