/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#include "SettingsDialog.h"
#include "ui_ProxyDialog.h"
#include "ui_Settings_Accounts.h"
#include "ui_Settings_Collection.h"
#include "ui_Settings_Advanced.h"

#include "config.h"

#include "AtticaManager.h"
#include "AclRegistry.h"
#include "TomahawkApp.h"
#include "TomahawkSettings.h"
#include "accounts/DelegateConfigWrapper.h"
#include "Pipeline.h"
#include "Resolver.h"
#include "ExternalResolverGui.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/GuiHelpers.h"
#include "accounts/AccountDelegate.h"
#include "database/Database.h"
#include "network/Servent.h"
#include "utils/AnimatedSpinner.h"
#include "accounts/AccountModel.h"
#include "accounts/Account.h"
#include "accounts/AccountManager.h"
#include "accounts/AccountModelFilterProxy.h"
#include "accounts/ResolverAccount.h"
#include "utils/Logger.h"
#include "accounts/AccountFactoryWrapper.h"
#include "accounts/spotify/SpotifyAccount.h"
#include "thirdparty/Qocoa/qtoolbartabdialog.h"
#include "thirdparty/Qocoa/qbutton.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkConfiguration>
#include <QNetworkProxy>
#include <QVBoxLayout>
#include <QSizeGrip>
#include <QToolBar>

using namespace Tomahawk;
using namespace Accounts;

SettingsDialog::SettingsDialog(QObject *parent )
    : QObject( parent )
    , m_accountsWidgetUi( new Ui_Settings_Accounts )
    , m_accountsWidget( new QWidget )
    , m_collectionWidgetUi( new Ui_Settings_Collection )
    , m_collectionWidget( new QWidget )
    , m_advancedWidgetUi( new Ui_Settings_Advanced )
    , m_advancedWidget( new QWidget )
    , m_proxySettings( 0 )
    , m_rejected( false )
    , m_restartRequired( false )
    , m_accountModel( 0 )
    , m_sipSpinner( 0 )
{
    m_accountsWidgetUi->setupUi( m_accountsWidget );
    m_collectionWidgetUi->setupUi( m_collectionWidget );
    m_advancedWidgetUi->setupUi( m_advancedWidget );

    m_accountsWidgetUi->accountsFilterCombo->setFocusPolicy( Qt::NoFocus );

    m_dialog = new QToolbarTabDialog;

    TomahawkSettings* s = TomahawkSettings::instance();

    m_advancedWidgetUi->checkBoxReporter->setChecked( s->crashReporterEnabled() );
    m_advancedWidgetUi->checkBoxHttp->setChecked( s->httpEnabled() );
    m_advancedWidgetUi->checkBoxSongChangeNotifications->setChecked( s->songChangeNotificationEnabled() );
    #ifndef Q_OS_LINUX // no backends on OSX or Win so far
        m_advancedWidgetUi->checkBoxSongChangeNotifications->setVisible( false );
    #endif

    //Network settings
    TomahawkSettings::ExternalAddressMode mode = TomahawkSettings::instance()->externalAddressMode();
    if ( mode == TomahawkSettings::Lan )
        m_advancedWidgetUi->lanOnlyRadioButton->setChecked( true );
    else if ( mode == TomahawkSettings::Static )
        m_advancedWidgetUi->staticIpRadioButton->setChecked( true );
    else
        m_advancedWidgetUi->upnpRadioButton->setChecked( true );

    m_advancedWidgetUi->staticHostNamePortLabel->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
    m_advancedWidgetUi->staticHostName->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
    m_advancedWidgetUi->staticPort->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
    m_advancedWidgetUi->staticHostNameLabel->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
    m_advancedWidgetUi->staticPortLabel->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );

    bool useProxy = TomahawkSettings::instance()->proxyType() == QNetworkProxy::Socks5Proxy;
    m_advancedWidgetUi->enableProxyCheckBox->setChecked( useProxy );
    m_advancedWidgetUi->proxyButton->setEnabled( useProxy );

    m_advancedWidgetUi->aclEntryClearButton->setEnabled( TomahawkSettings::instance()->aclEntries().size() > 0 );
    connect( m_advancedWidgetUi->aclEntryClearButton, SIGNAL( clicked( bool ) ), this, SLOT( aclEntryClearButtonClicked() ) );

#ifdef Q_WS_MAC
    // Avoid resize handles on sheets on osx
    m_proxySettings.setSizeGripEnabled( true );
    QSizeGrip* p = m_proxySettings.findChild< QSizeGrip* >();
    p->setFixedSize( 0, 0 );
#endif

    m_accountsWidgetUi->installFromFileBtn->setText( tr( "Install from file" ) );

    // Accounts
    AccountDelegate* accountDelegate = new AccountDelegate( this );
    m_accountsWidgetUi->accountsView->setItemDelegate( accountDelegate );
    m_accountsWidgetUi->accountsView->setContextMenuPolicy( Qt::CustomContextMenu );
    m_accountsWidgetUi->accountsView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    m_accountsWidgetUi->accountsView->setMouseTracking( true );

    connect( accountDelegate, SIGNAL( openConfig( Tomahawk::Accounts::Account* ) ), this, SLOT( openAccountConfig( Tomahawk::Accounts::Account* ) ) );
    connect( accountDelegate, SIGNAL( openConfig( Tomahawk::Accounts::AccountFactory* ) ), this, SLOT( openAccountFactoryConfig( Tomahawk::Accounts::AccountFactory* ) ) );
    connect( accountDelegate, SIGNAL( update( QModelIndex ) ), m_accountsWidgetUi->accountsView, SLOT( update( QModelIndex ) ) );

    m_accountModel = new AccountModel( this );
    m_accountProxy = new AccountModelFilterProxy( m_accountModel );
    m_accountProxy->setSourceModel( m_accountModel );

    connect( m_accountProxy, SIGNAL( startInstalling( QPersistentModelIndex ) ), accountDelegate, SLOT( startInstalling(QPersistentModelIndex) ) );
    connect( m_accountProxy, SIGNAL( doneInstalling( QPersistentModelIndex ) ), accountDelegate, SLOT( doneInstalling(QPersistentModelIndex) ) );
    connect( m_accountProxy, SIGNAL( errorInstalling( QPersistentModelIndex ) ), accountDelegate, SLOT( errorInstalling(QPersistentModelIndex) ) );
    connect( m_accountProxy, SIGNAL( scrollTo( QModelIndex ) ), this, SLOT( scrollTo( QModelIndex ) ) );

    m_accountsWidgetUi->accountsView->setModel( m_accountProxy );

    connect( m_accountsWidgetUi->installFromFileBtn, SIGNAL( clicked( bool ) ), this, SLOT( installFromFile() ) );
    connect( m_accountModel, SIGNAL( createAccount( Tomahawk::Accounts::AccountFactory* ) ), this, SLOT( createAccountFromFactory( Tomahawk::Accounts::AccountFactory* ) ) );

    m_accountsWidgetUi->accountsFilterCombo->addItem( tr( "All" ), Accounts::NoType );
    m_accountsWidgetUi->accountsFilterCombo->addItem( accountTypeToString( SipType ), SipType );
    m_accountsWidgetUi->accountsFilterCombo->addItem( accountTypeToString( ResolverType ), ResolverType );
    m_accountsWidgetUi->accountsFilterCombo->addItem( accountTypeToString( StatusPushType ), StatusPushType );

    connect( m_accountsWidgetUi->accountsFilterCombo, SIGNAL( activated( int ) ), this, SLOT( accountsFilterChanged( int ) ) );

    if ( !Servent::instance()->isReady() )
    {
        m_sipSpinner = new AnimatedSpinner( m_accountsWidgetUi->accountsView );
        m_sipSpinner->fadeIn();

        connect( Servent::instance(), SIGNAL( ready() ), this, SLOT( serventReady() ) );
    }

    // ADVANCED
    m_advancedWidgetUi->staticHostName->setText( s->externalHostname() );
    m_advancedWidgetUi->staticPort->setValue( s->externalPort() );
    m_advancedWidgetUi->proxyButton->setVisible( true );

    m_collectionWidgetUi->checkBoxWatchForChanges->setChecked( s->watchForChanges() );
    m_collectionWidgetUi->scannerTimeSpinBox->setValue( s->scannerTime() );
    m_collectionWidgetUi->enableEchonestCatalog->setChecked( s->enableEchonestCatalogs() );

    connect( m_collectionWidgetUi->checkBoxWatchForChanges, SIGNAL( clicked( bool ) ), SLOT( updateScanOptionsView() ) );

    if ( m_collectionWidgetUi->checkBoxWatchForChanges->isChecked() )
    {
        m_collectionWidgetUi->scanTimeLabel->show();
        m_collectionWidgetUi->scannerTimeSpinBox->show();
    }
    else
    {
        m_collectionWidgetUi->scanTimeLabel->hide();
        m_collectionWidgetUi->scannerTimeSpinBox->hide();
    }

    foreach ( const QString& dir, TomahawkSettings::instance()->scannerPaths() )
    {
        m_collectionWidgetUi->dirTree->checkPath( dir, Qt::Checked );
    }

    int buttonsWidth = qMax( m_advancedWidgetUi->proxyButton->sizeHint().width(),
                             m_advancedWidgetUi->aclEntryClearButton->sizeHint().width() );
    m_advancedWidgetUi->proxyButton->setFixedWidth( buttonsWidth );
    m_advancedWidgetUi->aclEntryClearButton->setFixedWidth( buttonsWidth );

#ifndef Q_OS_MAC
    m_advancedWidget->setMinimumSize( m_advancedWidget->sizeHint() );
    m_accountsWidget->setMinimumWidth( 500 );
#else
    m_accountsWidget->setContentsMargins( 6, 6, 6, 6 );
    m_accountsWidgetUi->horizontalLayout->setContentsMargins( 0, 0, 0, 0 );
    m_accountsWidgetUi->installFromFileBtn->setContentsMargins( -4, 0, 0, 0 );
    m_accountsWidget->setMinimumSize( 550, 400 );
    m_accountsWidgetUi->accountsView->setAttribute( Qt::WA_MacShowFocusRect, false );

    m_collectionWidget->setContentsMargins( 6, 6, 6, 6 );
    m_collectionWidget->setMinimumHeight( m_collectionWidgetUi->verticalLayout->sizeHint().height() + 20 );
    m_collectionWidgetUi->dirTree->setAttribute( Qt::WA_MacShowFocusRect, false );

    m_advancedWidget->setContentsMargins( 6, 6, 6, 6 );
    m_advancedWidget->setMinimumHeight( m_advancedWidgetUi->verticalLayout->sizeHint().height() );
#endif

    // NOW PLAYING
// #ifdef Q_WS_MAC
//     ui->checkBoxEnableAdium->setChecked( s->nowPlayingEnabled() );
// #else
//     ui->checkBoxEnableAdium->hide();
// #endif

    m_dialog->addTab( m_accountsWidget, TomahawkUtils::defaultPixmap( TomahawkUtils::AccountSettings ),
                      tr( "Services" ), tr( "Configure the accounts and services used by Tomahawk "
                                             "to search and retrieve music, find your friends and "
                                             "update your status." ) );

    m_dialog->addTab( m_collectionWidget, TomahawkUtils::defaultPixmap( TomahawkUtils::MusicSettings ),
                      tr( "Collection" ), tr( "Manage how Tomahawk finds music on your computer." ) );

    m_dialog->addTab( m_advancedWidget, TomahawkUtils::defaultPixmap( TomahawkUtils::AdvancedSettings ),
                      tr( "Advanced" ), tr( "Configure Tomahawk's advanced settings, including "
                                            "network connectivity settings, browser interaction "
                                            "and more." ) );

    m_dialog->setCurrentIndex( 0 );

    connect( m_advancedWidgetUi->proxyButton,  SIGNAL( clicked() ),  SLOT( showProxySettings() ) );
    connect( m_advancedWidgetUi->lanOnlyRadioButton, SIGNAL( toggled(bool) ), SLOT( requiresRestart() ) );
    connect( m_advancedWidgetUi->staticIpRadioButton, SIGNAL( toggled(bool) ), SLOT( requiresRestart() ) );
    connect( m_advancedWidgetUi->upnpRadioButton, SIGNAL( toggled(bool) ), SLOT( requiresRestart() ) );
    connect( m_advancedWidgetUi->lanOnlyRadioButton, SIGNAL( toggled(bool) ), SLOT( toggleRemoteMode() ) );
    connect( m_advancedWidgetUi->staticIpRadioButton, SIGNAL( toggled(bool) ), SLOT( toggleRemoteMode() ) );
    connect( m_advancedWidgetUi->upnpRadioButton, SIGNAL( toggled(bool) ), SLOT( toggleRemoteMode() ) );
    connect( m_advancedWidgetUi->enableProxyCheckBox, SIGNAL( toggled(bool) ), SLOT( toggleProxyEnabled() ) );

    connect( m_dialog, SIGNAL( accepted() ), SLOT( saveSettings() ) );
    connect( m_dialog, SIGNAL( rejected() ), SLOT( onRejected() ) );
}


void
SettingsDialog::saveSettings()
{
    qDebug() << Q_FUNC_INFO;

    TomahawkSettings* s = TomahawkSettings::instance();

    s->setCrashReporterEnabled( m_advancedWidgetUi->checkBoxReporter->checkState() == Qt::Checked );
    s->setHttpEnabled( m_advancedWidgetUi->checkBoxHttp->checkState() == Qt::Checked );
    s->setSongChangeNotificationEnabled( m_advancedWidgetUi->checkBoxSongChangeNotifications->checkState() == Qt::Checked );
    s->setProxyType( m_advancedWidgetUi->enableProxyCheckBox->isChecked() ? QNetworkProxy::Socks5Proxy : QNetworkProxy::NoProxy );
    s->setExternalAddressMode( m_advancedWidgetUi->upnpRadioButton->isChecked() ? TomahawkSettings::Upnp : ( m_advancedWidgetUi->lanOnlyRadioButton->isChecked() ? TomahawkSettings::Lan : TomahawkSettings::Static ) );

    s->setExternalHostname( m_advancedWidgetUi->staticHostName->text() );
    s->setExternalPort( m_advancedWidgetUi->staticPort->value() );

    s->setScannerPaths( m_collectionWidgetUi->dirTree->getCheckedPaths() );
    s->setWatchForChanges( m_collectionWidgetUi->checkBoxWatchForChanges->isChecked() );
    s->setScannerTime( m_collectionWidgetUi->scannerTimeSpinBox->value() );
    s->setEnableEchonestCatalogs( m_collectionWidgetUi->enableEchonestCatalog->isChecked() );

//         s->setNowPlayingEnabled( ui->checkBoxEnableAdium->isChecked() );

    s->applyChanges();
    s->sync();

    if ( m_restartRequired )
        QMessageBox::information( 0, tr( "Information" ), tr( "Some changed settings will not take effect until Tomahawk is restarted" ) );

    m_collectionWidgetUi->dirTree->cleanup();

    TomahawkUtils::NetworkProxyFactory* proxyFactory = TomahawkUtils::proxyFactory();
    if ( !m_advancedWidgetUi->enableProxyCheckBox->isChecked() )
    {
        tDebug() << Q_FUNC_INFO << "Got NoProxy selected";
        proxyFactory->setProxy( QNetworkProxy::NoProxy );
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "Got Socks5Proxy selected";
        proxyFactory->setProxy( QNetworkProxy( QNetworkProxy::Socks5Proxy, s->proxyHost(), s->proxyPort(), s->proxyUsername(), s->proxyPassword() ) );
        if ( !s->proxyNoProxyHosts().isEmpty() )
        {
            tDebug() << Q_FUNC_INFO << "noproxy hosts:" << s->proxyNoProxyHosts();
            tDebug() << Q_FUNC_INFO << "split noproxy line edit is " << s->proxyNoProxyHosts().split( ' ', QString::SkipEmptyParts );
            proxyFactory->setNoProxyHosts( s->proxyNoProxyHosts().split( ' ', QString::SkipEmptyParts ) );
        }
    }

}


SettingsDialog::~SettingsDialog()
{
    m_accountsWidget->deleteLater();
    m_collectionWidget->deleteLater();
    m_advancedWidget->deleteLater();
    m_dialog->deleteLater();
}


void
SettingsDialog::show()
{
    m_dialog->setCurrentIndex( 0 );
    m_dialog->show();
}

void
SettingsDialog::serventReady()
{
    m_sipSpinner->fadeOut();
}


void
SettingsDialog::onRejected()
{
    m_rejected = true;
}


void
SettingsDialog::changeEvent( QEvent *e )
{
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            m_accountsWidgetUi->retranslateUi( m_accountsWidget );
            m_collectionWidgetUi->retranslateUi( m_collectionWidget );
            m_advancedWidgetUi->retranslateUi( m_advancedWidget );
            break;

        default:
            break;
    }
}


void
SettingsDialog::showProxySettings()
{
    m_proxySettings.exec();
    if ( m_proxySettings.result() == QDialog::Accepted )
        m_proxySettings.saveSettings();
}


void
SettingsDialog::toggleRemoteMode()
{
    m_advancedWidgetUi->staticHostNamePortLabel->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
    m_advancedWidgetUi->staticHostName->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
    m_advancedWidgetUi->staticPort->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
    m_advancedWidgetUi->staticHostNameLabel->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
    m_advancedWidgetUi->staticPortLabel->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
}


void
SettingsDialog::toggleProxyEnabled()
{
    m_advancedWidgetUi->proxyButton->setEnabled( m_advancedWidgetUi->enableProxyCheckBox->isChecked() );
}


void
SettingsDialog::updateScanOptionsView()
{
    if ( m_collectionWidgetUi->checkBoxWatchForChanges->isChecked() )
    {
        m_collectionWidgetUi->scanTimeLabel->show();
        m_collectionWidgetUi->scannerTimeSpinBox->show();
    }
    else
    {
        m_collectionWidgetUi->scanTimeLabel->hide();
        m_collectionWidgetUi->scannerTimeSpinBox->hide();
    }
}


void
SettingsDialog::accountsFilterChanged( int )
{
    AccountType filter = static_cast< AccountType >( m_accountsWidgetUi->accountsFilterCombo->itemData( m_accountsWidgetUi->accountsFilterCombo->currentIndex() ).toInt() );
    m_accountProxy->setFilterType( filter );
}


void
SettingsDialog::openAccountFactoryConfig( AccountFactory* factory )
{
    QList< Account* > accts;
    foreach ( Account* acct, AccountManager::instance()->accounts() )
    {
        if ( AccountManager::instance()->factoryForAccount( acct ) == factory )
            accts << acct;
        if ( accts.size() > 1 )
            break;
    }
    Q_ASSERT( accts.size() > 0 ); // Shouldn't have a config wrench if there are no accounts!
    if ( accts.size() == 1 )
    {
        // If there's just one, open the config directly w/ the delete button. Otherwise open the multi dialog
        openAccountConfig( accts.first(), true );
        return;
    }

#ifndef Q_OS_MAC
    AccountFactoryWrapper dialog( factory, 0 );
    QWeakPointer< AccountFactoryWrapper > watcher( &dialog );

    dialog.exec();
#else
    // on osx a sheet needs to be non-modal
    AccountFactoryWrapper* dialog = new AccountFactoryWrapper( factory, 0 );
    dialog->show();
#endif
}


void
SettingsDialog::createAccountFromFactory( AccountFactory* factory )
{
    TomahawkUtils::createAccountFromFactory( factory, 0 );
}


void
SettingsDialog::openAccountConfig( Account* account, bool showDelete )
{
    TomahawkUtils::openAccountConfig( account, 0, showDelete );
}


void
SettingsDialog::installFromFile()
{
    const QString resolver = QFileDialog::getOpenFileName( 0, tr( "Install resolver from file" ), TomahawkSettings::instance()->scriptDefaultPath() );

    if( !resolver.isEmpty() )
    {
        const QFileInfo resolverAbsoluteFilePath( resolver );
        TomahawkSettings::instance()->setScriptDefaultPath( resolverAbsoluteFilePath.absolutePath() );

        if ( resolverAbsoluteFilePath.baseName() == "spotify_tomahawkresolver" )
        {
            // HACK if this is a spotify resolver, we treat is specially.
            // usually we expect the user to just download the spotify resolver from attica,
            // however developers, those who build their own tomahawk, can't do that, or linux
            // users can't do that. However, we have an already-existing SpotifyAccount that we
            // know exists that we need to use this resolver path.
            //
            // Hence, we special-case the spotify resolver and directly set the path on it here.
            SpotifyAccount* acct = 0;
            foreach ( Account* account, AccountManager::instance()->accounts() )
            {
                if ( SpotifyAccount* spotify = qobject_cast< SpotifyAccount* >( account ) )
                {
                    acct = spotify;
                    break;
                }
            }

            if ( acct )
            {
                acct->setManualResolverPath( resolver );
                return;
            }
        }

        Account* acct = AccountManager::instance()->accountFromPath( resolver );

        AccountManager::instance()->addAccount( acct );
        TomahawkSettings::instance()->addAccount( acct->accountId() );
        AccountManager::instance()->enableAccount( acct );
    }
}


void
SettingsDialog::aclEntryClearButtonClicked()
{
    QMessageBox::StandardButton button = QMessageBox::question(
                           0,
                           tr( "Delete all Access Control entries?" ),
                           tr( "Do you really want to delete all Access Control entries? You will be asked for a decision again for each peer that you connect to." ),
                           QMessageBox::Ok | QMessageBox::Cancel,
                           QMessageBox::Ok
                         );
    if ( button == QMessageBox::Ok )
    {
        ACLRegistry::instance()->wipeEntries();
        m_advancedWidgetUi->aclEntryClearButton->setEnabled( false );
    }
}


void
SettingsDialog::scrollTo( const QModelIndex& idx )
{
    m_accountsWidgetUi->accountsView->scrollTo( idx, QAbstractItemView::PositionAtBottom );
}


void
SettingsDialog::requiresRestart()
{
    m_restartRequired = true;
}


ProxyDialog::ProxyDialog( QWidget *parent )
: QDialog( parent )
, ui( new Ui::ProxyDialog )
{
    ui->setupUi( this );

    // ugly, I know, but...

    TomahawkSettings* s = TomahawkSettings::instance();

    ui->hostLineEdit->setText( s->proxyHost() );
    ui->portSpinBox->setValue( s->proxyPort() );
    ui->userLineEdit->setText( s->proxyUsername() );
    ui->passwordLineEdit->setText( s->proxyPassword() );
    ui->checkBoxUseProxyForDns->setChecked( s->proxyDns() );
    ui->noHostLineEdit->setText( s->proxyNoProxyHosts() );
}


void
ProxyDialog::saveSettings()
{
    qDebug() << Q_FUNC_INFO;

    //First set settings
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setProxyHost( ui->hostLineEdit->text() );

    int port = ui->portSpinBox->value();
    s->setProxyPort( port );
    s->setProxyNoProxyHosts( ui->noHostLineEdit->text() );
    s->setProxyUsername( ui->userLineEdit->text() );
    s->setProxyPassword( ui->passwordLineEdit->text() );
    s->setProxyDns( ui->checkBoxUseProxyForDns->checkState() == Qt::Checked );
    s->sync();
}
