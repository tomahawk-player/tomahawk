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
#include "xmppconfigwidget.h"
#include "ui_xmppconfigwidget.h"

#include "utils/tomahawkutils.h"

#include <QtPlugin>
#include <QInputDialog>

using namespace Tomahawk;
using namespace Accounts;

Account*
GoogleWrapperFactory::createAccount( const QString& pluginId )
{
    return new GoogleWrapper( pluginId.isEmpty() ? generateId( factoryId() ) : pluginId );
}


QIcon
GoogleWrapperFactory::icon() const
{
    return QIcon( ":/gmail-logo.png" );
}

GoogleWrapperSip::GoogleWrapperSip( Account* account )
    : XmppSipPlugin( account )
{

}

GoogleWrapperSip::~GoogleWrapperSip()
{
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
    config->m_ui->headerLabel->setText( tr( "Configure this Google Account" ) );
    config->m_ui->emailLabel->setText( tr( "Google Address" ) );
    config->m_ui->xmppBlurb->setText( tr( "Enter your Google login to connect with your friends using Tomahawk!" ) );
    config->m_ui->logoLabel->setPixmap( QPixmap( ":/gmail-logo.png" ) );
    config->m_ui->xmppServer->setText( "talk.google.com" );
    config->m_ui->xmppPort->setValue( 5222 );
    config->m_ui->groupBoxXmppAdvanced->hide();
}

GoogleWrapper::~GoogleWrapper()
{
    delete m_sipPlugin.data();
}


QIcon
GoogleWrapper::icon() const
{
    return QIcon( ":/gmail-logo.png" );
}


SipPlugin*
GoogleWrapper::sipPlugin()
{
    if ( m_xmppSipPlugin.isNull() )
    {
        m_xmppSipPlugin = QWeakPointer< XmppSipPlugin >( new GoogleWrapperSip( const_cast< GoogleWrapper* >( this ) ) );

        connect( m_xmppSipPlugin.data(), SIGNAL( stateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), this, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) );
        connect( m_xmppSipPlugin.data(), SIGNAL( error( int, QString ) ), this, SIGNAL( error( int, QString ) ) );

        return m_xmppSipPlugin.data();
    }
    return m_xmppSipPlugin.data();
}


#ifdef GOOGLE_WRAPPER
Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::GoogleWrapperFactory )
#endif
