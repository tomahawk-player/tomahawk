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
#include "tomahawksettings.h"

#include <QtGui/QFileDialog>

GetNewStuffDialog::GetNewStuffDialog( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
    , ui( new Ui::GetNewStuffDialog )
    , m_model( new GetNewStuffModel( this ) )
{
    ui->setupUi( this );

    ui->accountsList->setModel( m_model );
    GetNewStuffDelegate* del = new GetNewStuffDelegate( ui->accountsList );
    connect( del, SIGNAL( update( QModelIndex ) ), ui->accountsList, SLOT( update( QModelIndex ) ) );
    ui->accountsList->setItemDelegate( del );
    ui->accountsList->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

    ui->accountsList->setMouseTracking( true );

    setMinimumSize( 560, 350 );

#ifdef Q_WS_MAC
    setMaximumSize( 560, 350 );
    setSizeGripEnabled( false );

    ui->accountsList->setAttribute( Qt::WA_MacShowFocusRect, false );
#endif

    connect( ui->installFromFileBtn, SIGNAL( clicked( bool ) ), this, SLOT( installFromFile() ) );
}


GetNewStuffDialog::~GetNewStuffDialog()
{
    delete ui;
}


void
GetNewStuffDialog::installFromFile()
{
    QString resolver = QFileDialog::getOpenFileName( this, tr( "Load script resolver file" ), TomahawkSettings::instance()->scriptDefaultPath() );

//     m_resolversModel->addResolver( resolver, true );
    // TODO
    if( !resolver.isEmpty() )
    {

        QFileInfo resolverAbsoluteFilePath = resolver;
        TomahawkSettings::instance()->setScriptDefaultPath( resolverAbsoluteFilePath.absolutePath() );
    }
}
