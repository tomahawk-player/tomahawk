/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "googlewrapper.h"
#include "ui_configwidget.h"

#include <QtPlugin>
#include <QInputDialog>


SipPlugin*
GoogleWrapperFactory::createPlugin( const QString& pluginId )
{
    return new GoogleWrapper( pluginId.isEmpty() ? generateId() : pluginId );
}


QIcon
GoogleWrapperFactory::icon() const
{
    return QIcon( ":/gmail-logo.png" );
}


GoogleWrapper::GoogleWrapper ( const QString& pluginID )
    : JabberPlugin ( pluginID )
{
    m_ui->headerLabel->setText( tr( "Configure this Google Account" ) );
    m_ui->emailLabel->setText( tr( "Google Address" ) );
    m_ui->jabberBlurb->setText( tr( "Enter your Google login to connect with your friends using Tomahawk!" ) );
    m_ui->logoLabel->setPixmap( QPixmap( ":/gmail-logo.png" ) );
    m_ui->jabberServer->setText( "talk.google.com" );
    m_ui->jabberPort->setValue( 5222 );
    m_ui->groupBoxJabberAdvanced->hide();
}


QIcon
GoogleWrapper::icon() const
{
    return QIcon( ":/gmail-logo.png" );
}


QString
GoogleWrapper::defaultSuffix() const
{
    return "@gmail.com";
}


void
GoogleWrapper::showAddFriendDialog()
{
    bool ok;
    QString id = QInputDialog::getText( 0, tr( "Add Friend" ),
                                        tr( "Enter Google Address:" ), QLineEdit::Normal,
                                            "", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to add google contact to roster:" << id;
    addContact( id );
}


#ifdef GOOGLE_WRAPPER
Q_EXPORT_PLUGIN2( sipfactory, GoogleWrapperFactory )
#endif
