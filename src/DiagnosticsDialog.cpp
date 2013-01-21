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
#include "TomahawkVersion.h"

#include "SourceList.h"

#include "accounts/AccountManager.h"
#include "network/Servent.h"
#include "sip/PeerInfo.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "infosystem/InfoSystem.h"
#include "infosystem/InfoSystemWorker.h"

#include <QLabel>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QDebug>


DiagnosticsDialog::DiagnosticsDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::DiagnosticsDialog )
{
    ui->setupUi( this );

    connect( ui->clipboardButton, SIGNAL( clicked() ), SLOT( copyToClipboard() ) );
    connect( ui->logfileButton, SIGNAL( clicked() ), SLOT( openLogfile() ) );
    connect( ui->buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );

    updateLogView();
}


void
DiagnosticsDialog::updateLogView()
{
    QString log;

    log.append( QString( "TOMAHAWK DIAGNOSTICS LOG -%1 \n\n" ).arg( QDateTime::currentDateTime().toString() ) );
    log.append( "TOMAHAWK-VERSION: " TOMAHAWK_VERSION "\n" );
    log.append( "PLATFORM: " TOMAHAWK_SYSTEM "\n\n");
    log.append( "NETWORK:\n    General:\n" );

    if ( Servent::instance()->visibleExternally() )
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
        log.append( "      visible: false\n" );
    }

    log.append( "\n\nINFOPLUGINS:\n" );
    QThread* infoSystemWorkerThreadSuperClass = Tomahawk::InfoSystem::InfoSystem::instance()->workerThread();
    Tomahawk::InfoSystem::InfoSystemWorkerThread* infoSystemWorkerThread = qobject_cast< Tomahawk::InfoSystem::InfoSystemWorkerThread* >(infoSystemWorkerThreadSuperClass);

    foreach(const Tomahawk::InfoSystem::InfoPluginPtr& plugin, infoSystemWorkerThread->worker()->plugins())
    {
        log.append("      ");
        log.append( plugin->friendlyName() );
        log.append("\n");
    }

    log.append( "\n\n" );

    log.append( "ACCOUNTS:\n" );

    const QList< Tomahawk::source_ptr > sources = SourceList::instance()->sources( true );
    const QList< Tomahawk::Accounts::Account* > accounts = Tomahawk::Accounts::AccountManager::instance()->accounts( Tomahawk::Accounts::SipType );
    foreach ( Tomahawk::Accounts::Account* account, accounts )
    {
        Q_ASSERT( account && account->sipPlugin() );
        if ( !account || !account->sipPlugin() )
            continue;

        connect( account, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), SLOT( updateLogView() ), Qt::UniqueConnection );
        connect( account, SIGNAL( error( int, QString ) ), SLOT( updateLogView() ), Qt::UniqueConnection );
        connect( account->sipPlugin(), SIGNAL( peerStatusChanged( Tomahawk::peerinfo_ptr ) ), SLOT( updateLogView() ), Qt::UniqueConnection );

        log.append( accountLog( account ) + "\n" );
    }

    ui->text->setText( log );
}


void
DiagnosticsDialog::copyToClipboard()
{
    QApplication::clipboard()->setText( ui->text->toPlainText() );
}


void
DiagnosticsDialog::openLogfile()
{
    TomahawkUtils::openUrl( Logger::logFile() );
}


QString
DiagnosticsDialog::accountLog( Tomahawk::Accounts::Account* account )
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

    foreach( const Tomahawk::peerinfo_ptr& peerInfo, account->sipPlugin()->peersOnline() )
    {
        QString peerId = peerInfo->id();
        QString versionString = peerInfo->versionString();
        SipInfo sipInfo = peerInfo->sipInfo();
        if ( !sipInfo.isValid() )
        {
            accountInfo.append(
                QString("       %1: %2 %3" /*"(%4)"*/)
                    .arg( peerInfo->id() )
                    .arg( "sipinfo invalid" )
                    .arg( versionString )
                    // .arg( connected ? "connected" : "not connected")
            );
        }
        else if ( sipInfo.isVisible() )
        {
            accountInfo.append(
                QString("       %1: %2:%3 %4" /*" (%5)"*/)
                    .arg( peerId )
                    .arg( sipInfo.host() )
                    .arg( sipInfo.port() )
                    .arg( versionString )
                    // .arg( connected ? "connected" : "not connected")
            );
        }
        else
        {
            accountInfo.append(
                QString("       %1: visible: false %2" /*" (%3)"*/)
                    .arg( peerId )
                    .arg( versionString )
                    // .arg( connected ? "connected" : "not connected")
            );
        }

        if( sipInfo.isValid() )
        {
            if( !Servent::instance()->visibleExternally() ||
                Servent::instance()->externalAddress() < sipInfo.host() ||
            ( Servent::instance()->externalAddress() == sipInfo.host() && Servent::instance()->externalPort() < sipInfo.port() ) )
            {
                accountInfo.append(" (outbound)");
            }
            else
            {
                accountInfo.append(" (inbound)");
            }
        }
        accountInfo.append("\n");
    }
    accountInfo.append( "\n" );

    return accountInfo;
}
