/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "GetNewStuffDialog.h"

#include "ui_GetNewStuffDialog.h"
#include "GetNewStuffDelegate.h"
#include "GetNewStuffModel.h"

GetNewStuffDialog::GetNewStuffDialog( QWidget *parent, Qt::WindowFlags f )
    : QDialog( parent, f )
    , ui( new Ui::GetNewStuffDialog )
    , m_model( new GetNewStuffModel( this ) )
{
    ui->setupUi( this );

    ui->listView->setModel( m_model );
    ui->listView->setItemDelegate( new GetNewStuffDelegate( ui->listView ) );

#ifdef Q_WS_MAC
    setMinimumSize( 510, 350 );
    setMaximumSize( 510, 350 );
    setSizeGripEnabled( false );

    ui->listView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    ui->listView->setAttribute( Qt::WA_MacShowFocusRect, false );
#endif

}

GetNewStuffDialog::~GetNewStuffDialog()
{
    delete ui;
}
