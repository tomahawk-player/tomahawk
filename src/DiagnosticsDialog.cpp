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

#include <sip/SipHandler.h>
#include <network/Servent.h>
#include <SourceList.h>

#include <QTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>

#include "utils/Logger.h"


DiagnosticsDialog::DiagnosticsDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::DiagnosticsDialog )
{
    ui->setupUi( this );

    connect( ui->updateButton, SIGNAL( clicked() ), this, SLOT( updateLogView() ) );
    connect( ui->clipboardButton, SIGNAL( clicked() ), this, SLOT( copyToClipboard() ) );
    connect( ui->buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );

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
        "TOMAHAWK-VERSION: " TOMAHAWK_VERSION "\n\n\n"
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
    log.append("\n\n");


    // Peers / Accounts, TODO
    log.append("ACCOUNTS:\n");
//     QList< Tomahawk::source_ptr > sources = SourceList::instance()->sources( true );
//     Q_FOREACH(SipPlugin *sip, SipHandler::instance()->allPlugins())
//     {
//         Q_ASSERT(sip);
//         QString stateString;
//         switch( sip->connectionState() )
//         {
//             case SipPlugin::Connecting:
//                 stateString = "Connecting";
//                 break;
//
//             case SipPlugin::Connected:
//                 stateString = "Connected";
//                 break;
//
//             case SipPlugin::Disconnected:
//                 stateString = "Disconnected";
//                 break;
//             case SipPlugin::Disconnecting:
//                 stateString = "Disconnecting";
//         }
//         log.append(
//             QString("  %2 (%1): %3 (%4)\n")
//                 .arg(sip->account()->accountServiceName())
//                 .arg(sip->friendlyName())
//                 .arg(sip->account()->accountFriendlyName())
//                 .arg(stateString)
//         );

//         Q_FOREACH( const QString &peerId, sip->peersOnline() )
//         {
//             /* enable this again, when we check the Source.has this peerId
//             bool connected = false;
//             Q_FOREACH( const Tomahawk::source_ptr &source, sources )
//             {
//                 if( source->controlConnection() )
//                 {
//                     connected = true;
//                     break;
//                 }
//             }*/
//
//             QString versionString = SipHandler::instance()->versionString( peerId );
//             SipInfo sipInfo = SipHandler::instance()->sipInfo( peerId );
//             if( !sipInfo.isValid() )
//                log.append(
//                     QString("       %1: %2 %3" /*"(%4)"*/ "\n")
//                         .arg( peerId )
//                         .arg( "sipinfo invalid" )
//                         .arg( versionString )
//                         // .arg( connected ? "connected" : "not connected")
//                 );
//             else if( sipInfo.isVisible() )
//                 log.append(
//                     QString("       %1: %2:%3 %4" /*" (%5)"*/ "\n")
//                         .arg( peerId )
//                         .arg( sipInfo.host().hostName() )
//                         .arg( sipInfo.port() )
//                         .arg( versionString )
//                         // .arg( connected ? "connected" : "not connected")
//
//                 );
//             else
//                 log.append(
//                     QString("       %1: visible: false %2" /*" (%3)"*/ "\n")
//                         .arg( peerId )
//                         .arg( versionString )
//                         // .arg( connected ? "connected" : "not connected")
//
//                 );
//         }
//         log.append("\n");
//     }

    ui->logView->setPlainText(log);
}

void DiagnosticsDialog::copyToClipboard()
{
    QApplication::clipboard()->setText( ui->logView->toPlainText() );
}

