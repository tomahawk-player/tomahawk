/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "XmppAccount.h"
#include "XmppConfigWidget.h"
#include "ui_XmppConfigWidget.h"


#include "accounts/AccountManager.h"
#include "utils/Logger.h"

#include <QMessageBox>

namespace Tomahawk
{

namespace Accounts
{

XmppConfigWidget::XmppConfigWidget( XmppAccount* account, QWidget *parent )
    : AccountConfigWidget( parent )
    , m_ui( new Ui::XmppConfigWidget )
    , m_account( account )
    , m_disableChecksForGoogle( false )
{
    m_ui->setupUi( this );

    m_ui->xmppUsername->setText( account->credentials().contains( "username" ) ? account->credentials()[ "username" ].toString() : QString() );
    m_ui->xmppPassword->setText( account->credentials().contains( "password" ) ? account->credentials()[ "password" ].toString() : QString() );
    m_ui->xmppServer->setText( account->configuration().contains( "server" ) ? account->configuration()[ "server" ].toString() : QString() );
    m_ui->xmppPort->setValue( account->configuration().contains( "port" ) ? account->configuration()[ "port" ].toInt() : 5222 );
    m_ui->xmppPublishTracksCheckbox->setChecked( account->configuration().contains( "publishtracks" ) ? account->configuration()[ "publishtracks" ].toBool() : true);
    m_ui->xmppEnforceSecureCheckbox->setChecked( account->configuration().contains( "enforcesecure" ) ? account->configuration()[ "enforcesecure" ].toBool() : false);
    m_ui->jidExistsLabel->hide();


    connect( m_ui->xmppUsername, SIGNAL( textChanged( QString ) ), SLOT( onCheckJidExists( QString ) ) );
}


XmppConfigWidget::~XmppConfigWidget()
{
    delete m_ui;
}

void
XmppConfigWidget::saveConfig()
{
    QVariantHash credentials = m_account->credentials();
    credentials[ "username" ] = m_ui->xmppUsername->text().trimmed();
    credentials[ "password" ] = m_ui->xmppPassword->text().trimmed();

    QVariantHash configuration = m_account->configuration();
    configuration[ "server" ] = m_ui->xmppServer->text().trimmed();
    configuration[ "port" ] = m_ui->xmppPort->text().trimmed();
    configuration[ "publishtracks"] = m_ui->xmppPublishTracksCheckbox->isChecked();
    configuration[ "enforcesecure"] = m_ui->xmppEnforceSecureCheckbox->isChecked();

    m_account->setAccountFriendlyName( m_ui->xmppUsername->text() );
    m_account->setCredentials( credentials );
    m_account->setConfiguration( configuration);
    m_account->sync();

    static_cast< XmppSipPlugin* >( m_account->sipPlugin() )->checkSettings();
}


void
XmppConfigWidget::onCheckJidExists( const QString &jid )
{
    QList< Tomahawk::Accounts::Account* > accounts = Tomahawk::Accounts::AccountManager::instance()->accounts( Tomahawk::Accounts::SipType );
    foreach( Tomahawk::Accounts::Account* account, accounts )
    {
        if ( account->accountId() == m_account->accountId() )
            continue;

        QString savedUsername = account->credentials()[ "username" ].toString();
        QStringList savedSplitUsername = account->credentials()[ "username" ].toString().split("@");
        QString savedServer = account->configuration()[ "server" ].toString();
        int savedPort = account->configuration()[ "port" ].toInt();

        if ( ( savedUsername == jid || savedSplitUsername.contains( jid ) ) &&
               savedServer == m_ui->xmppServer->text() &&
               savedPort == m_ui->xmppPort->value() &&
               !jid.trimmed().isEmpty() )
        {
            m_ui->jidExistsLabel->show();
            // the already jid exists
            emit dataError( true );
            return;
        }
    }
    m_ui->jidExistsLabel->hide();
    emit dataError( false );
}


void
XmppConfigWidget::checkForErrors()
{
    const QString username = m_ui->xmppUsername->text().trimmed();
    const QStringList usernameParts = username.split( '@', QString::KeepEmptyParts );

    QString errorMessage;
    if( username.isEmpty() )
    {
        errorMessage.append( tr( "You forgot to enter your username!" ) );
    }

    //HACK: don't check for xmpp id being an "email address"
    if( !m_disableChecksForGoogle )
    {
        if( usernameParts.count() != 2 || usernameParts.at( 0 ).isEmpty() || ( usernameParts.count() == 2 && usernameParts.at( 1 ).isEmpty() ) )
        {
            errorMessage.append( tr( "Your Xmpp Id should look like an email address" ) );
        }
    }

    if( !errorMessage.isEmpty() )
    {
        errorMessage.append( tr( "\n\nExample:\nusername@jabber.org" ) );
        m_errors.append( errorMessage );
    }
}


}

}
