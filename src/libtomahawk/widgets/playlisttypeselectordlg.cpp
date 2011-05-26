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

#include "widgets/newplaylistwidget.h"
#include "viewmanager.h"
#include "viewpage.h"
#include "sourcelist.h"

#include "playlisttypeselectordlg.h"
#include "ui_playlisttypeselectordlg.h"


PlaylistTypeSelectorDlg::PlaylistTypeSelectorDlg( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
    , ui( new Ui::PlaylistTypeSelectorDlg )
{
    ui->setupUi( this );

#ifdef Q_WS_MAC
//    ui->
    ui->horizontalLayout_2->setContentsMargins( 4, 4, 4, 4 );

    setSizeGripEnabled( false );
    setMinimumSize( size() );
    setMaximumSize( size() ); // to remove the resize grip on osx this is the only way
#endif

    m_isAutoPlaylist = false;
    m_playlistName = "";

    connect( ui->manualPlaylistButton, SIGNAL( clicked() ),
             this, SLOT( createNormalPlaylist() ));
    connect( ui->autoPlaylistButton, SIGNAL( clicked() ),
             this, SLOT( createAutomaticPlaylist() ));
    connect( ui->autoPlaylistNameLine, SIGNAL( textChanged( const QString& )),
             this, SLOT( enableAutoPlaylistButton( const QString& )));
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

void PlaylistTypeSelectorDlg::createAutomaticPlaylist() { m_isAutoPlaylist = true;
    m_playlistName = ui->autoPlaylistNameLine->text();
    done( QDialog::Accepted ); // return code is used to vaidate we did not exit out of the Dialog successfully
}

QString
PlaylistTypeSelectorDlg::playlistName() const
{
    return m_playlistName;
}

bool
PlaylistTypeSelectorDlg::playlistTypeIsAuto() const
{
    return m_isAutoPlaylist;
}

void
PlaylistTypeSelectorDlg::enableAutoPlaylistButton( const QString &text )
{
    ui->autoPlaylistButton->setEnabled( !text.isEmpty() );
}

