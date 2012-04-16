/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#include "AboutDialog.h"

#include "ui_AboutDialog.h"

#include "utils/tomahawkutils.h"
#include "config.h"

#include <QString>
#include <QCoreApplication>

AboutDialog::AboutDialog( QWidget* parent )
    : QDialog( parent )
    , m_ui( new Ui::AboutDialog )
{
    m_ui->setupUi( this );

    const QString version = TomahawkUtils::appFriendlyVersion();
    QString debugHash;
#ifdef DEBUG_BUILD
    debugHash = QString( "<br />(%1)" ).arg( qApp->applicationVersion() );
#endif

    m_ui->titleLabel->setText( m_ui->titleLabel->text().arg( version ).arg( debugHash ) );
}
