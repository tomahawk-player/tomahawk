/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *   Copyright 2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "DiagnosticsDialog.h"
#include "ui_DiagnosticsDialog.h"

#include "config.h"

#include "accounts/AccountManager.h"
#include "network/Servent.h"
#include "SourceList.h"

#include <QLabel>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QDebug>

#include "utils/Logger.h"
#include "sip/SipHandler.h"


DiagnosticsDialog::DiagnosticsDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::DiagnosticsDialog )
{
    ui->setupUi( this );

    //connect( ui->updateButton, SIGNAL( clicked() ), this, SLOT( updateLogView() ) );
    connect( ui->clipboardButton, SIGNAL( clicked() ), this, SLOT( copyToClipboard() ) );
    connect( ui->buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );

    ui->scrollAreaWidgetContents->setLayout( new QVBoxLayout() );
    
    updateLogView();
}

void DiagnosticsDialog::updateLogView()
{
    QString log;

    log.append(
        QString("TOMAHAWK DIAGNOSTICS LOG -%1 \n\n")
            .arg( QDateTime::currentDateTime().toString() )
    );

    // network
    log.append(
        "TOMAHAWK-VERSION: " TOMAHAWK_VERSION "\n\n"
    );

    // network
    log.append(
        "NETWORK:\n"
        "    General:\n"
    );
    if( Servent::instance()->visibleExternally() )
    {
        log.append(
            QString(
                "      visible: true\n"
                "      host: %1\n"
                "      port: %2\n"
                "\n"
            ).arg( Servent::instance()->externalAddress() )
             .arg( Servent::instance()->externalPort() )

        );
    }
    else
    {
        log.append(
            QString(
                "      visible: false"
            )
        );
    }
    //log.append("\n\n");

    ui->scrollAreaWidgetContents->layout()->addWidget( new QLabel( log, this ) );
    
    // Peers / Accounts, TODO
    ui->scrollAreaWidgetContents->layout()->addWidget( new QLabel( "ACCOUNTS:\n", this ) );
    
    QString accountInfo;
    
    const QList< Tomahawk::source_ptr > sources = SourceList::instance()->sources( true );
    const QList< Tomahawk::Accounts::Account* > accounts = Tomahawk::Accounts::AccountManager::instance()->accounts( Tomahawk::Accounts::SipType );
    foreach ( Tomahawk::Accounts::Account* account, accounts )
    {
        Q_ASSERT( account && account->sipPlugin() );
        if ( !account || !account->sipPlugin() )
            continue;

        connect( account, SIGNAL(connectionStateChanged(Tomahawk::Accounts::Account::ConnectionState)), SLOT(onAccountConnectionStateChanged(Tomahawk::Accounts::Account::ConnectionState)) );
        connect( account, SIGNAL(error(int,QString)), SLOT(onAccountError(int,QString)) );
        connect( account->sipPlugin(), SIGNAL(peerOnline(QString)), SLOT(onPeerOnline(QString)) );
        connect( account->sipPlugin(), SIGNAL(peerOffline(QString)), SLOT(onPeerOffline(QString)) );
        connect( account->sipPlugin(), SIGNAL(sipInfoReceived(QString,SipInfo)), SLOT(onSipInfoReceived(QString,SipInfo)) );
        connect( account->sipPlugin(), SIGNAL(softwareVersionReceived(QString,QString)), SLOT(onSoftwareVersionReceived(QString,QString)) );
        
        QString stateString;
        switch( account->connectionState() )
        {
            case Tomahawk::Accounts::Account::Connecting:
                stateString = "Connecting";
                break;
            case Tomahawk::Accounts::Account::Connected:
                stateString = "Connected";
                break;

            case Tomahawk::Accounts::Account::Disconnected:
                stateString = "Disconnected";
                break;
            case Tomahawk::Accounts::Account::Disconnecting:
                stateString = "Disconnecting";
        }
        accountInfo.append(
            QString( "  %2 (%1): %3 (%4)\n" )
                .arg( account->accountServiceName() )
                .arg( account->sipPlugin()->friendlyName() )
                .arg( account->accountFriendlyName())
                .arg( stateString )
        );

        foreach( const QString &peerId, account->sipPlugin()->peersOnline() )
        {
            /* enable this again, when we check the Source.has this peerId
            bool connected = false;
            Q_FOREACH( const Tomahawk::source_ptr &source, sources )
            {
                if( source->controlConnection() )
                {
                    connected = true;
                    break;
                }
            }*/

            QString versionString = SipHandler::instance()->versionString( peerId );
            SipInfo sipInfo = SipHandler::instance()->sipInfo( peerId );
            if( !sipInfo.isValid() )
               accountInfo.append(
                    QString("       %1: %2 %3" /*"(%4)"*/ "\n")
                        .arg( peerId )
                        .arg( "sipinfo invalid" )
                        .arg( versionString )
                        // .arg( connected ? "connected" : "not connected")
                );
            else if( sipInfo.isVisible() )
                accountInfo.append(
                    QString("       %1: %2:%3 %4" /*" (%5)"*/ "\n")
                        .arg( peerId )
                        .arg( sipInfo.host().hostName() )
                        .arg( sipInfo.port() )
                        .arg( versionString )
                        // .arg( connected ? "connected" : "not connected")

                );
            else
                accountInfo.append(
                    QString("       %1: visible: false %2" /*" (%3)"*/ "\n")
                        .arg( peerId )
                        .arg( versionString )
                        // .arg( connected ? "connected" : "not connected")

                );
        }
        accountInfo.append("\n");
        
        QLabel *accountInfoLabel = new QLabel( accountInfo, this );
        
        ui->scrollAreaWidgetContents->layout()->addWidget( accountInfoLabel );
        
        m_accountDescriptionStore.insert( account, accountInfoLabel );
        
        accountInfo.clear();
    }
    //ui->logView->setPlainText(log);
}

void DiagnosticsDialog::copyToClipboard()
{
    //QApplication::clipboard()->setText( ui->logView->toPlainText() );
}

void DiagnosticsDialog::onAccountConnectionStateChanged(Tomahawk::Accounts::Account::ConnectionState state)
{
    Tomahawk::Accounts::Account* account = qobject_cast< Tomahawk::Accounts::Account* >( sender() );
    Q_ASSERT( account );
    
    updateAccountLabel( account );    
}

void DiagnosticsDialog::onAccountError(int errorId, QString errorString)
{
    Tomahawk::Accounts::Account* account = qobject_cast< Tomahawk::Accounts::Account* >( sender() );
    Q_ASSERT( account );
}

void DiagnosticsDialog::onPeerOnline(const QString& )
{
    Tomahawk::Accounts::Account* account = qobject_cast< SipPlugin* >( sender() )->account();
    Q_ASSERT(account);
    
    updateAccountLabel( account );
}

void DiagnosticsDialog::onPeerOffline(const QString& )
{
    Tomahawk::Accounts::Account* account = qobject_cast< SipPlugin* >( sender() )->account();
    Q_ASSERT(account);
    
    updateAccountLabel( account );
}

void DiagnosticsDialog::onSipInfoReceived(const QString& peerId, const SipInfo& info)
{
    Tomahawk::Accounts::Account* account = qobject_cast< SipPlugin* >( sender() )->account();
    Q_ASSERT(account);
    
    updateAccountLabel( account );
}

void DiagnosticsDialog::onSoftwareVersionReceived(const QString& peerId, const QString& versionString)
{
    Tomahawk::Accounts::Account* account = qobject_cast< SipPlugin* >( sender() )->account();
    Q_ASSERT(account);
    
    updateAccountLabel( account );
}

void DiagnosticsDialog::updateAccountLabel( Tomahawk::Accounts::Account* account)
{
    QLabel* accountInfoLabel = m_accountDescriptionStore.value(account);
    
    if( accountInfoLabel )
    {
        QString accountInfo;
        QString stateString;
        switch( account->connectionState() )
        {
            case Tomahawk::Accounts::Account::Connecting:
                stateString = "Connecting";
                break;
            case Tomahawk::Accounts::Account::Connected:
                stateString = "Connected";
                break;

            case Tomahawk::Accounts::Account::Disconnected:
                stateString = "Disconnected";
                break;
            case Tomahawk::Accounts::Account::Disconnecting:
                stateString = "Disconnecting";
        }
        accountInfo.append(
            QString( "  %2 (%1): %3 (%4)\n" )
                .arg( account->accountServiceName() )
                .arg( account->sipPlugin()->friendlyName() )
                .arg( account->accountFriendlyName())
                .arg( stateString )
        );

        foreach( const QString &peerId, account->sipPlugin()->peersOnline() )
        {
            QString versionString = SipHandler::instance()->versionString( peerId );
            SipInfo sipInfo = SipHandler::instance()->sipInfo( peerId );
            if( !sipInfo.isValid() )
                accountInfo.append(
                    QString("       %1: %2 %3" /*"(%4)"*/ "\n")
                        .arg( peerId )
                        .arg( "sipinfo invalid" )
                        .arg( versionString )
                        // .arg( connected ? "connected" : "not connected")
                );
            else if( sipInfo.isVisible() )
                accountInfo.append(
                    QString("       %1: %2:%3 %4" /*" (%5)"*/ "\n")
                        .arg( peerId )
                        .arg( sipInfo.host().hostName() )
                        .arg( sipInfo.port() )
                        .arg( versionString )
                        // .arg( connected ? "connected" : "not connected")

                );
            else
                accountInfo.append(
                    QString("       %1: visible: false %2" /*" (%3)"*/ "\n")
                        .arg( peerId )
                        .arg( versionString )
                        // .arg( connected ? "connected" : "not connected")

                );
        }
        accountInfo.append("\n");
        
        accountInfoLabel->setText( accountInfo );
    }
}