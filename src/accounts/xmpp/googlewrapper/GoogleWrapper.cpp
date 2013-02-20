/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>
    Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>    

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


#include "GoogleWrapper.h"
#include "../XmppConfigWidget.h"
#include "ui_XmppConfigWidget.h"

#include "utils/TomahawkUtilsGui.h"

#include <QtPlugin>
#include <QInputDialog>

using namespace Tomahawk;
using namespace Accounts;

Account*
GoogleWrapperFactory::createAccount( const QString& pluginId )
{
    return new GoogleWrapper( pluginId.isEmpty() ? generateId( factoryId() ) : pluginId );
}


QPixmap
GoogleWrapperFactory::icon() const
{
    return QPixmap( ":/google-account/gmail-logo.png" );
}

GoogleWrapperSip::GoogleWrapperSip( Account* account )
    : XmppSipPlugin( account )
{

}

GoogleWrapperSip::~GoogleWrapperSip()
{
}

QString
GoogleWrapperSip::inviteString() const
{
    return tr( "Enter Google Address" );
}


void
GoogleWrapperSip::showAddFriendDialog()
{
    bool ok;
    QString id = QInputDialog::getText( TomahawkUtils::tomahawkWindow(), tr( "Add Friend" ),
                                        tr( "Enter Google Address:" ), QLineEdit::Normal, "", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to add google contact to roster:" << id;
    addContact( id );
}


QString
GoogleWrapperSip::defaultSuffix() const
{
    return "@gmail.com";
}


GoogleWrapper::GoogleWrapper ( const QString& pluginID )
    : XmppAccount ( pluginID )
{
    XmppConfigWidget* config = static_cast< XmppConfigWidget* >( m_configWidget.data() );
    config->m_disableChecksForGoogle = true;
    config->m_ui->headerLabel->setText( tr( "Configure this Google Account" ) );
    config->m_ui->emailLabel->setText( tr( "Google Address:" ) );
    config->m_ui->xmppBlurb->setText( tr( "Enter your Google login to connect with your friends using Tomahawk!" ) );
    config->m_ui->xmppUsername->setPlaceholderText( tr( "username@gmail.com" ) );
    config->m_ui->logoLabel->setPixmap( QPixmap( ":/google-account/gmail-logo.png" ) );
    config->m_ui->xmppServer->setText( "talk.google.com" );
    config->m_ui->xmppPort->setValue( 5222 );
    config->m_ui->groupBoxXmppAdvanced->hide();

    m_onlinePixmap = QPixmap( ":/google-account/gmail-logo.png" );
    m_offlinePixmap = QPixmap( ":/google-account/gmail-offline-logo.png" );
}

GoogleWrapper::~GoogleWrapper()
{
    delete m_sipPlugin.data();
}


SipPlugin*
GoogleWrapper::sipPlugin()
{
    if ( m_xmppSipPlugin.isNull() )
    {
        m_xmppSipPlugin = QPointer< XmppSipPlugin >( new GoogleWrapperSip( const_cast< GoogleWrapper* >( this ) ) );

        connect( m_xmppSipPlugin.data(), SIGNAL( stateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), this, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) );
        connect( m_xmppSipPlugin.data(), SIGNAL( error( int, QString ) ), this, SIGNAL( error( int, QString ) ) );

        return m_xmppSipPlugin.data();
    }
    return m_xmppSipPlugin.data();
}


#ifdef GOOGLE_WRAPPER
Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::GoogleWrapperFactory )
#endif
