/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christopher Reichert <creichert07@gmail.com>
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

#include "PlaylistTypeSelectorDialog.h"
#include "ui_PlaylistTypeSelectorDialog.h"

#include "widgets/NewPlaylistWidget.h"
#include "ViewManager.h"
#include "ViewPage.h"
#include "SourceList.h"

#include "utils/Logger.h"


PlaylistTypeSelectorDlg::PlaylistTypeSelectorDlg( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
    , ui( new Ui::PlaylistTypeSelectorDlg )
{
    ui->setupUi( this );

#ifdef Q_WS_MAC
//    ui->
    ui->verticalLayout->setContentsMargins( 4, 0, 4, 4 );

    setSizeGripEnabled( false );
    resize( width(), 180 );
    setMinimumSize( size() );
    setMaximumSize( size() ); // to remove the resize grip on osx this is the only way
    setContentsMargins( 12, 12, 12, 12 );
#else
    ui->verticalLayout->setContentsMargins( 9, 0, 9, 9 );
#endif

    ui->line->setMaximumHeight( ui->label->sizeHint().height() );
    ui->line->setContentsMargins( 0, 0, 0, 0 );
    m_isAutoPlaylist = false;

    connect( ui->manualPlaylistButton, SIGNAL( clicked() ),
             this, SLOT( createNormalPlaylist() ));
    connect( ui->autoPlaylistButton, SIGNAL( clicked() ),
             this, SLOT( createAutomaticPlaylist() ));
}


PlaylistTypeSelectorDlg::~PlaylistTypeSelectorDlg()
{
    delete ui;
}


void
PlaylistTypeSelectorDlg::createNormalPlaylist()
{
    m_isAutoPlaylist = false;
    done( QDialog::Accepted ); // return code is used to vaidate we did not exit out of the Dialog
}


void
PlaylistTypeSelectorDlg::createAutomaticPlaylist()
{
    m_isAutoPlaylist = true;
    done( QDialog::Accepted ); // return code is used to vaidate we did not exit out of the Dialog successfully
}


QString
PlaylistTypeSelectorDlg::playlistName() const
{
    return ui->playlistNameLine->text();
}


bool
PlaylistTypeSelectorDlg::playlistTypeIsAuto() const
{
    return m_isAutoPlaylist;
}
