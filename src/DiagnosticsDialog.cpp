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

#include <QLabel>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QDebug>

#include "accounts/AccountManager.h"
#include "network/Servent.h"
#include "SourceList.h"

#include "sip/SipHandler.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"


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
    log.append( "TOMAHAWK-VERSION: " TOMAHAWK_VERSION "\n\n" );
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
        log.append( "      visible: false" );
    }

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
        connect( account->sipPlugin(), SIGNAL( peerOnline( QString ) ), SLOT( updateLogView() ), Qt::UniqueConnection );
        connect( account->sipPlugin(), SIGNAL( peerOffline( QString ) ), SLOT( updateLogView() ), Qt::UniqueConnection );
        connect( account->sipPlugin(), SIGNAL( sipInfoReceived( QString, SipInfo ) ), SLOT( updateLogView() ), Qt::UniqueConnection );
        connect( account->sipPlugin(), SIGNAL( softwareVersionReceived( QString, QString ) ), SLOT( updateLogView() ), Qt::UniqueConnection );

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

    foreach( const QString& peerId, account->sipPlugin()->peersOnline() )
    {
        QString versionString = SipHandler::instance()->versionString( peerId );
        SipInfo sipInfo = SipHandler::instance()->sipInfo( peerId );
        if ( !sipInfo.isValid() )
        {
            accountInfo.append(
                QString("       %1: %2 %3" /*"(%4)"*/ "\n")
                    .arg( peerId )
                    .arg( "sipinfo invalid" )
                    .arg( versionString )
                    // .arg( connected ? "connected" : "not connected")
            );
        }
        else if ( sipInfo.isVisible() )
        {
            accountInfo.append(
                QString("       %1: %2:%3 %4" /*" (%5)"*/ "\n")
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
                QString("       %1: visible: false %2" /*" (%3)"*/ "\n")
                    .arg( peerId )
                    .arg( versionString )
                    // .arg( connected ? "connected" : "not connected")
            );
        }
    }
    accountInfo.append( "\n" );

    return accountInfo;
}