/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *   Copyright 2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "infosystem/InfoSystem.h"
#include "infosystem/InfoSystemWorker.h"
#include "network/Servent.h"
#include "sip/PeerInfo.h"
#include "sip/SipInfo.h"
#include "sip/SipPlugin.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "Pipeline.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>


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

    log.append( QString( "%applicationName DIAGNOSTICS LOG -%1 \n\n" ).arg( QDateTime::currentDateTime().toString() ) );
    log.append( "TOMAHAWK-VERSION: " TOMAHAWK_VERSION "\n" );
    log.append( "PLATFORM: " TOMAHAWK_SYSTEM "\n");
    log.append( QString( "DBID: %1\n\n" ).arg( Tomahawk::Database::instance()->impl()->dbid() ) );
    log.append( "NETWORK:\n    Listening to:\n" );

    if ( Servent::instance()->visibleExternally() )
    {
        foreach ( QHostAddress ha, Servent::instance()->addresses() )
        {
            if ( ha.protocol() == QAbstractSocket::IPv6Protocol )
                log.append( QString( "      [%1]:%2\n" ).arg( ha.toString() ).arg( Servent::instance()->port() ) );
            else
                log.append( QString( "      %1:%2\n" ).arg( ha.toString() ).arg( Servent::instance()->port() ) );
        }
        if ( !Servent::instance()->additionalAddress().isNull() )
        {
            log.append( QString( "      [%1]:%2\n" ).arg( Servent::instance()->additionalAddress() ).arg( Servent::instance()->additionalPort() ) );
        }

    }
    else
    {
        log.append( "      not listening to any interface, outgoing connections only\n" );
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


    log.append( "RESOLVERS:\n" );


    connect( Tomahawk::Pipeline::instance(), SIGNAL( resolverAdded( Tomahawk::Resolver* ) ), SLOT( updateLogView() ), Qt::UniqueConnection );
    connect( Tomahawk::Pipeline::instance(), SIGNAL( resolverRemoved( Tomahawk::Resolver* ) ), SLOT( updateLogView() ), Qt::UniqueConnection );

    const QList< Tomahawk::Resolver* > resolvers = Tomahawk::Pipeline::instance()->resolvers();
    foreach ( Tomahawk::Resolver* resolver, resolvers )
    {

        log.append( resolver->name() + "\n" );
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
    TomahawkUtils::openUrl( TomahawkUtils::logFilePath() );
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

    QMap< QString, QList< Tomahawk::peerinfo_ptr > > nodes;
    foreach ( const Tomahawk::peerinfo_ptr& peerInfo, account->sipPlugin()->peersOnline() )
    {
        if ( peerInfo->sipInfos().isEmpty() )
        {
            accountInfo.append( QString( "       %1: waiting for SIP information\n" ).arg( peerInfo->id() ) );
        }
        else if ( peerInfo->nodeId().isEmpty() )
        {
            QList< Tomahawk::peerinfo_ptr> infos;
            infos.append( peerInfo );
            accountInfo.append( peerLog( peerInfo->nodeId(), infos ) );
        }
        else
        {
            if ( !nodes.contains( peerInfo->nodeId() ) )
            {
                nodes[peerInfo->nodeId()] = QList< Tomahawk::peerinfo_ptr >();
            }
            nodes[peerInfo->nodeId()].append( peerInfo);
        }
    }
    foreach ( const QString& nodeid, nodes.keys() )
    {
        accountInfo.append( peerLog( nodeid, nodes.value( nodeid ) ) );
    }
    accountInfo.append( "\n" );

    return accountInfo;
}

QString
DiagnosticsDialog::peerLog( const QString& nodeid, const QList<Tomahawk::peerinfo_ptr> &peerInfos )
{
    QString peerLine( "       " );
    QStringList peerIds;
    foreach ( const Tomahawk::peerinfo_ptr& peerInfo, peerInfos )
    {
        peerIds << peerInfo->id();
    }
    peerLine.append( peerIds.join( QChar( ',' ) ) );
    peerLine.append( QString( ": %1 @ ").arg( nodeid ) );
    QStringList sipInfos;
    foreach ( const Tomahawk::peerinfo_ptr& peerInfo, peerInfos )
    {
        foreach ( SipInfo info, peerInfo->sipInfos() )
        {
            if ( info.isValid() )
                sipInfos << QString( "[%1]:%2" ).arg( info.host() ).arg( info.port() ) ;
            else
                sipInfos << QString( "SipInfo invalid" );
        }
    }
    sipInfos.removeDuplicates();
    peerLine.append( sipInfos.join( QChar( ';' ) ) );
    if ( ( ( peerInfos.first()->sipInfos().length() == 1 ) && ( !peerInfos.first()->sipInfos().first().isVisible() ) ) || ( peerInfos.first()->sipInfos().isEmpty() ) )
        peerLine.append( "(outbound connections only) ");
    peerLine.append( QString( " (%1)\n" ).arg( peerInfos.first()->versionString() ) );
    return peerLine;
}
