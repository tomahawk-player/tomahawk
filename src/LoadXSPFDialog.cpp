/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "LoadXSPFDialog.h"
#include "ui_LoadXSPFDialog.h"

#include "TomahawkSettings.h"
#include "Source.h"

#include <QFileDialog>

LoadXSPFDialog::LoadXSPFDialog( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
    , m_ui( new Ui_LoadXSPF )
{
    m_ui->setupUi( this );

#ifdef Q_WS_MAC
    m_ui->horizontalLayout->setContentsMargins( 0, 0, 0, 0 );
    m_ui->horizontalLayout->setSpacing( 5 );
    m_ui->verticalLayout->setContentsMargins( 0, 10, 0, 0 );
    m_ui->verticalLayout->setSpacing( 0 );
#endif

    connect( m_ui->navigateButton, SIGNAL( clicked( bool ) ), this, SLOT( getLocalFile() ) );
}

LoadXSPFDialog::~LoadXSPFDialog()
{
}

void
LoadXSPFDialog::getLocalFile()
{
    const QString path = TomahawkSettings::instance()->importXspfPath();
    QString url = QFileDialog::getOpenFileName( this, tr( "Load XSPF File" ), path, tr( "XSPF Files (*.xspf)" ) );
    if ( !url.isEmpty() )
        TomahawkSettings::instance()->setImportXspfPath( QFileInfo( url ).absoluteDir().absolutePath() );

    m_ui->lineEdit->setText( url );
}

QString
LoadXSPFDialog::xspfUrl() const
{
    return m_ui->lineEdit->text();
}

bool
LoadXSPFDialog::autoUpdate() const
{
    return m_ui->autoUpdate->isChecked();
}
