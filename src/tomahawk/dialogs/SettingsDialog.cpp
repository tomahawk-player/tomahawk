/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012-2014, Teo Mrnjavac <teo@kde.org>
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
#include "ui_HostDialog.h"
#include "ui_ProxyDialog.h"
#include "ui_Settings_Accounts.h"
#include "ui_Settings_Collection.h"
#include "ui_Settings_Advanced.h"
#include "ui_Settings_Downloads.h"

#include "config.h"

#include "AtticaManager.h"
#include "network/acl/AclRegistry.h"
#include "GlobalActionManager.h"
#include "TomahawkApp.h"
#include "TomahawkSettings.h"
#include "accounts/DelegateConfigWrapper.h"
#include "Pipeline.h"
#include "resolvers/Resolver.h"
#include "resolvers/ExternalResolverGui.h"
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
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "utils/NetworkAccessManager.h"
#include "utils/NetworkProxyFactory.h"
#include "resolvers/JSAccount.h"
#include "resolvers/JSResolver.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkConfiguration>
#include <QNetworkProxy>
#include <QVBoxLayout>
#include <QSizeGrip>
#include <QToolBar>
#include <QMenu>

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
    , m_downloadsWidgetUi( new Ui_Settings_Downloads )
    , m_downloadsWidget( new QWidget )
    , m_staticHostSettings( 0 )
    , m_proxySettings( 0 )
    , m_restartRequired( false )
    , m_accountModel( 0 )
    , m_sipSpinner( 0 )
    , m_contextMenu( 0 )
{
    m_accountsWidget->setFont( TomahawkUtils::systemFont() );
    m_collectionWidget->setFont( TomahawkUtils::systemFont() );
    m_advancedWidget->setFont( TomahawkUtils::systemFont() );
    m_downloadsWidget->setFont( TomahawkUtils::systemFont() );

    m_accountsWidgetUi->setupUi( m_accountsWidget );
    m_collectionWidgetUi->setupUi( m_collectionWidget );
    m_advancedWidgetUi->setupUi( m_advancedWidget );
    m_downloadsWidgetUi->setupUi( m_downloadsWidget );

    m_accountsWidgetUi->accountsFilterCombo->setFocusPolicy( Qt::NoFocus );
    m_dialog = new QToolbarTabDialog;
    TomahawkSettings* s = TomahawkSettings::instance();

    m_advancedWidgetUi->checkBoxReporter->setChecked( s->crashReporterEnabled() );
    m_advancedWidgetUi->checkBoxHttp->setChecked( s->httpEnabled() );
    m_advancedWidgetUi->checkBoxListenApi->setChecked( s->httpBindAll() );
    m_advancedWidgetUi->checkBoxSongChangeNotifications->setChecked( s->songChangeNotificationEnabled() );

    //Network settings
    Tomahawk::Network::ExternalAddress::Mode mode = TomahawkSettings::instance()->externalAddressMode();
    if ( mode == Tomahawk::Network::ExternalAddress::Lan )
        m_advancedWidgetUi->lanOnlyRadioButton->setChecked( true );
    else if ( mode == Tomahawk::Network::ExternalAddress::Static )
        m_advancedWidgetUi->staticIpRadioButton->setChecked( true );
    else
        m_advancedWidgetUi->upnpRadioButton->setChecked( true );

    m_advancedWidgetUi->staticHostSettingsButton->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );

    bool useProxy = TomahawkSettings::instance()->proxyType() == QNetworkProxy::Socks5Proxy;
    m_advancedWidgetUi->enableProxyCheckBox->setChecked( useProxy );
    m_advancedWidgetUi->proxyButton->setEnabled( useProxy );

    m_advancedWidgetUi->vlcArgsLineEdit->setText( s->vlcArguments() );

    m_advancedWidgetUi->aclEntryClearButton->setEnabled( TomahawkSettings::instance()->aclEntries().size() > 0 );
    connect( m_advancedWidgetUi->aclEntryClearButton, SIGNAL( clicked( bool ) ), this, SLOT( aclEntryClearButtonClicked() ) );

#ifdef Q_OS_MAC
    // Avoid resize handles on sheets on osx
    m_proxySettings.setSizeGripEnabled( true );
    QSizeGrip* p = m_proxySettings.findChild< QSizeGrip* >();
    p->setFixedSize( 0, 0 );
    m_staticHostSettings.setSizeGripEnabled( true );
    p = m_staticHostSettings.findChild< QSizeGrip* >();
    p->setFixedSize( 0, 0 );
#endif

    m_accountsWidgetUi->installFromFileBtn->setText( tr( "Install Plug-In..." ) );

    // Accounts
    AccountDelegate* accountDelegate = new AccountDelegate( this );
    m_accountsWidgetUi->accountsView->setItemDelegate( accountDelegate );
    m_accountsWidgetUi->accountsView->setContextMenuPolicy( Qt::CustomContextMenu );
    m_accountsWidgetUi->accountsView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    m_accountsWidgetUi->accountsView->setMouseTracking( true );

    m_contextMenu = new QMenu( m_accountsWidgetUi->accountsView );
    m_contextMenu->setFont( TomahawkUtils::systemFont() );
    connect( m_accountsWidgetUi->accountsView, SIGNAL( customContextMenuRequested( QPoint ) ), SLOT( onCustomContextMenu( QPoint ) ) );
    QAction* showDebuggerAction = m_contextMenu->addAction( tr( "Open Account &Debugger..." ) );
    connect( showDebuggerAction, SIGNAL( triggered(bool) ), SLOT( onShowDebuggerForSelectedAccount() ) );

    connect( accountDelegate, SIGNAL( openConfig( Tomahawk::Accounts::Account* ) ), SLOT( openAccountConfig( Tomahawk::Accounts::Account* ) ) );
    connect( accountDelegate, SIGNAL( openConfig( Tomahawk::Accounts::AccountFactory* ) ), SLOT( openAccountFactoryConfig( Tomahawk::Accounts::AccountFactory* ) ) );
    connect( accountDelegate, SIGNAL( update( QModelIndex ) ), m_accountsWidgetUi->accountsView, SLOT( update( QModelIndex ) ) );

    m_accountModel = new AccountModel( this );
    m_accountProxy = new AccountModelFilterProxy( m_accountModel );
    m_accountProxy->setSourceModel( m_accountModel );

    connect( m_accountProxy, SIGNAL( startInstalling( QPersistentModelIndex ) ), accountDelegate, SLOT( startInstalling(QPersistentModelIndex) ) );
    connect( m_accountProxy, SIGNAL( doneInstalling( QPersistentModelIndex ) ), accountDelegate, SLOT( doneInstalling(QPersistentModelIndex) ) );
    connect( m_accountProxy, SIGNAL( errorInstalling( QPersistentModelIndex ) ), accountDelegate, SLOT( errorInstalling(QPersistentModelIndex) ) );
    connect( m_accountProxy, SIGNAL( scrollTo( QModelIndex ) ), SLOT( scrollTo( QModelIndex ) ) );

    m_accountsWidgetUi->accountsView->setModel( m_accountProxy );

    connect( m_accountsWidgetUi->installFromFileBtn, SIGNAL( clicked( bool ) ), SLOT( installFromFile() ) );
    connect( m_accountModel, SIGNAL( createAccount( Tomahawk::Accounts::AccountFactory* ) ), SLOT( createAccountFromFactory( Tomahawk::Accounts::AccountFactory* ) ) );

    m_accountsWidgetUi->accountsFilterCombo->addItem( tr( "All" ), Accounts::NoType );
    m_accountsWidgetUi->accountsFilterCombo->addItem( accountTypeToString( SipType ), SipType );
    m_accountsWidgetUi->accountsFilterCombo->addItem( accountTypeToString( ResolverType ), ResolverType );
    m_accountsWidgetUi->accountsFilterCombo->addItem( accountTypeToString( StatusPushType ), StatusPushType );

    connect( m_accountsWidgetUi->accountsFilterCombo, SIGNAL( activated( int ) ), SLOT( accountsFilterChanged( int ) ) );

    if ( !Servent::instance()->isReady() )
    {
        m_sipSpinner = new AnimatedSpinner( m_accountsWidgetUi->accountsView );
        m_sipSpinner->fadeIn();

        connect( Servent::instance(), SIGNAL( ready() ), SLOT( serventReady() ) );
    }

    // ADVANCED
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

/*    foreach ( const QString& dir, TomahawkSettings::instance()->scannerPaths() )
    {
        m_collectionWidgetUi->dirTree->checkPath( dir, Qt::Checked );
    }*/
    m_collectionWidgetUi->pathListWidget->addItems( TomahawkSettings::instance()->scannerPaths() );

    const int buttonSize = TomahawkUtils::defaultFontHeight() * 2.5;
    m_collectionWidgetUi->addLibraryPathButton->setFixedSize( buttonSize, buttonSize );
    m_collectionWidgetUi->removeLibraryPathButton->setFixedSize( m_collectionWidgetUi->addLibraryPathButton->size() );

    connect( m_collectionWidgetUi->addLibraryPathButton, SIGNAL( clicked() ), SLOT( addLibraryPath() ) );
    connect( m_collectionWidgetUi->removeLibraryPathButton, SIGNAL( clicked() ), SLOT( removeLibraryPath() ) );

    int buttonsWidth = qMax( m_advancedWidgetUi->proxyButton->sizeHint().width(),
                             m_advancedWidgetUi->aclEntryClearButton->sizeHint().width() );
    m_advancedWidgetUi->proxyButton->setFixedWidth( buttonsWidth );
    m_advancedWidgetUi->aclEntryClearButton->setFixedWidth( buttonsWidth );

    m_downloadsWidgetUi->downloadsFolder->setText( TomahawkSettings::instance()->downloadsPath() );
    connect( m_downloadsWidgetUi->pickFolderButton, SIGNAL( clicked() ), SLOT( pickDownloadsPath() ) );

    m_downloadsFormats.insert( "MP3", tr( "MP3" ) );
    m_downloadsFormats.insert( "FLAC", tr( "FLAC" ) );
    m_downloadsFormats.insert( "M4A", tr( "M4A" ) );
    m_downloadsFormats.insert( "MP4", tr( "MP4" ) );
    foreach ( const QString& format, m_downloadsFormats.values() )
    {
        m_downloadsWidgetUi->preferredFormatComboBox->addItem( format );
    }
    int i = m_downloadsWidgetUi->preferredFormatComboBox->findText( m_downloadsFormats.value( TomahawkSettings::instance()->downloadsPreferredFormat() ) );
    if ( i < 0 )
        i = m_downloadsWidgetUi->preferredFormatComboBox->findText( "MP3" );
    m_downloadsWidgetUi->preferredFormatComboBox->setCurrentIndex( i );

#ifndef Q_OS_MAC
    m_advancedWidget->setMinimumSize( m_advancedWidget->sizeHint() );
    m_accountsWidget->setMinimumWidth( 500 );
#else
    m_accountsWidget->setContentsMargins( 6, 6, 6, 6 );
    m_accountsWidgetUi->horizontalLayout->setContentsMargins( 0, 0, 0, 0 );
    m_accountsWidgetUi->installFromFileBtn->setContentsMargins( 0, 0, 0, 0 );
    m_accountsWidget->setMinimumSize( 550, 400 );
    m_accountsWidgetUi->accountsView->setAttribute( Qt::WA_MacShowFocusRect, false );

    m_collectionWidget->setContentsMargins( 6, 6, 6, 6 );
    m_collectionWidget->setMinimumHeight( m_collectionWidgetUi->verticalLayout->sizeHint().height() + 20 );
    m_collectionWidgetUi->pathListWidget->setAttribute( Qt::WA_MacShowFocusRect, false );

    m_advancedWidget->setContentsMargins( 6, 6, 6, 6 );
    m_advancedWidget->setMinimumHeight( m_advancedWidgetUi->verticalLayout->sizeHint().height() );
#endif

    // NOW PLAYING
// #ifdef Q_OS_MAC
//     ui->checkBoxEnableAdium->setChecked( s->nowPlayingEnabled() );
// #else
//     ui->checkBoxEnableAdium->hide();
// #endif

    m_dialog->addTab( m_accountsWidget, TomahawkUtils::defaultPixmap( TomahawkUtils::AccountSettings ),
                      tr( "Plug-Ins" ), tr( "Configure the accounts and services used by %applicationName "
                                             "to search and retrieve music, find your friends and "
                                             "update your status." ) );

    m_dialog->addTab( m_collectionWidget, TomahawkUtils::defaultPixmap( TomahawkUtils::MusicSettings ),
                      tr( "Collection" ), tr( "Manage how %applicationName finds music on your computer." ) );

    m_dialog->addTab( m_advancedWidget, TomahawkUtils::defaultPixmap( TomahawkUtils::AdvancedSettings ),
                      tr( "Advanced" ), tr( "Configure %applicationName advanced settings, including "
                                            "network connectivity settings, browser interaction "
                                            "and more." ) );

    m_dialog->addTab( m_downloadsWidget, TomahawkUtils::defaultPixmap( TomahawkUtils::DownloadsSettings ),
                      tr( "Downloads" ), tr( "Configure %applicationName's integrated download manager." ) );

    m_dialog->setCurrentIndex( 0 );

    connect( m_advancedWidgetUi->staticHostSettingsButton, SIGNAL( clicked() ), SLOT( showStaticHostSettings() ) );
    connect( m_advancedWidgetUi->proxyButton,  SIGNAL( clicked() ), SLOT( showProxySettings() ) );
    connect( m_advancedWidgetUi->lanOnlyRadioButton, SIGNAL( toggled( bool ) ), SLOT( requiresRestart() ) );
    connect( m_advancedWidgetUi->staticIpRadioButton, SIGNAL( toggled( bool ) ), SLOT( requiresRestart() ) );
    connect( m_advancedWidgetUi->upnpRadioButton, SIGNAL( toggled( bool ) ), SLOT( requiresRestart() ) );
    connect( m_advancedWidgetUi->lanOnlyRadioButton, SIGNAL( toggled( bool ) ), SLOT( toggleRemoteMode() ) );
    connect( m_advancedWidgetUi->staticIpRadioButton, SIGNAL( toggled( bool ) ), SLOT( toggleRemoteMode() ) );
    connect( m_advancedWidgetUi->upnpRadioButton, SIGNAL( toggled( bool ) ), SLOT( toggleRemoteMode() ) );
    connect( m_advancedWidgetUi->enableProxyCheckBox, SIGNAL( toggled( bool ) ), SLOT( toggleProxyEnabled() ) );
    connect( m_advancedWidgetUi->enableProxyCheckBox, SIGNAL( toggled( bool ) ), SLOT( requiresRestart() ) );

    connect( m_dialog, SIGNAL( accepted() ), SLOT( saveSettings() ) );
    connect( m_dialog, SIGNAL( rejected() ), SLOT( onRejected() ) );

    // Echonest is dead, make catalog upload checkbox invisible
    m_collectionWidgetUi->enableEchonestCatalog->setVisible( false );
}


void
SettingsDialog::saveSettings()
{
    qDebug() << Q_FUNC_INFO;

    TomahawkSettings* s = TomahawkSettings::instance();

    s->setCrashReporterEnabled( m_advancedWidgetUi->checkBoxReporter->checkState() == Qt::Checked );
    s->setHttpEnabled( m_advancedWidgetUi->checkBoxHttp->checkState() == Qt::Checked );
    s->setHttpBindAll( m_advancedWidgetUi->checkBoxListenApi->checkState() == Qt::Checked );
    s->setSongChangeNotificationEnabled( m_advancedWidgetUi->checkBoxSongChangeNotifications->checkState() == Qt::Checked );
    s->setProxyType( m_advancedWidgetUi->enableProxyCheckBox->isChecked() ? QNetworkProxy::Socks5Proxy : QNetworkProxy::NoProxy );
    s->setExternalAddressMode( m_advancedWidgetUi->upnpRadioButton->isChecked() ? Tomahawk::Network::ExternalAddress::Upnp : ( m_advancedWidgetUi->lanOnlyRadioButton->isChecked() ? Tomahawk::Network::ExternalAddress::Lan : Tomahawk::Network::ExternalAddress::Static ) );

    QStringList libraryPaths;
    for ( int i = 0; i < m_collectionWidgetUi->pathListWidget->count(); i++ )
    {
        libraryPaths << m_collectionWidgetUi->pathListWidget->item( i )->text();
    }
    s->setScannerPaths( libraryPaths );
//    s->setScannerPaths( m_collectionWidgetUi->dirTree->getCheckedPaths() );
    s->setWatchForChanges( m_collectionWidgetUi->checkBoxWatchForChanges->isChecked() );
    s->setScannerTime( m_collectionWidgetUi->scannerTimeSpinBox->value() );
    s->setEnableEchonestCatalogs( m_collectionWidgetUi->enableEchonestCatalog->isChecked() );
    s->setDownloadsPath( m_downloadsWidgetUi->downloadsFolder->text() );
    s->setDownloadsPreferredFormat( m_downloadsFormats.key( m_downloadsWidgetUi->preferredFormatComboBox->currentText() ) );

//         s->setNowPlayingEnabled( ui->checkBoxEnableAdium->isChecked() );

    s->applyChanges();
    s->sync();

    m_restartRequired = m_restartRequired || m_advancedWidgetUi->vlcArgsLineEdit->text() != s->vlcArguments();
    s->setVlcArguments( m_advancedWidgetUi->vlcArgsLineEdit->text() );

    if ( m_restartRequired )
        QMessageBox::information( 0, tr( "Information" ), tr( "Some changed settings will not take effect until %applicationName is restarted" ) );

//    m_collectionWidgetUi->dirTree->cleanup();

    Tomahawk::Utils::NetworkProxyFactory* proxyFactory = Tomahawk::Utils::proxyFactory();
    if ( !m_advancedWidgetUi->enableProxyCheckBox->isChecked() )
    {
        tDebug() << Q_FUNC_INFO << "Got NoProxy selected";
        proxyFactory->setProxy( QNetworkProxy::NoProxy, s->proxyDns() );
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "Got Socks5Proxy selected";
        proxyFactory->setProxy( QNetworkProxy( QNetworkProxy::Socks5Proxy, s->proxyHost(), s->proxyPort(), s->proxyUsername(), s->proxyPassword() ), s->proxyDns() );
        if ( !s->proxyNoProxyHosts().isEmpty() )
        {
            tDebug() << Q_FUNC_INFO << "noproxy hosts:" << s->proxyNoProxyHosts();
            tDebug() << Q_FUNC_INFO << "split noproxy line edit is " << s->proxyNoProxyHosts().split( ' ', QString::SkipEmptyParts );
            proxyFactory->setNoProxyHosts( s->proxyNoProxyHosts().split( ' ', QString::SkipEmptyParts ) );
        }
    }

    emit finished( true );
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
    emit finished( false );
}


void
SettingsDialog::onCustomContextMenu( const QPoint& point )
{
    QModelIndex index = m_accountsWidgetUi->accountsView->indexAt( point );
    if ( !index.isValid() )
        return;

    // HACK until there is a proper ScriptAccount
    ResolverAccount* account = qobject_cast< ResolverAccount* >( m_accountProxy->data( index, AccountModel::AccountData ).value< Tomahawk::Accounts::Account* >() );
    if ( !account )
        return;
    Tomahawk::JSResolver* jsResolver = qobject_cast< Tomahawk::JSResolver* >( account->resolver() );
    if ( !jsResolver )
        return;

    m_contextMenu->exec( m_accountsWidgetUi->accountsView->mapToGlobal( point ) );
}


void
SettingsDialog::onShowDebuggerForSelectedAccount()
{
    ResolverAccount* account = qobject_cast< ResolverAccount* >( m_accountProxy->data( m_accountsWidgetUi->accountsView->currentIndex(), AccountModel::AccountData ).value< Tomahawk::Accounts::Account* >() );
    Tomahawk::JSResolver* jsResolver = qobject_cast< Tomahawk::JSResolver* >( account->resolver() );
    jsResolver->scriptAccount()->showDebugger();
}


void
SettingsDialog::changeEvent( QEvent *e )
{
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
        {
            m_accountsWidgetUi->retranslateUi( m_accountsWidget );
            m_collectionWidgetUi->retranslateUi( m_collectionWidget );
            m_advancedWidgetUi->retranslateUi( m_advancedWidget );
            break;
        }

        default:
            break;
    }
}


void
SettingsDialog::showStaticHostSettings()
{
    m_staticHostSettings.exec();
    if ( m_staticHostSettings.result() == QDialog::Accepted )
    {
        requiresRestart();
        m_staticHostSettings.saveSettings();
    }
}


void
SettingsDialog::showProxySettings()
{
    m_proxySettings.exec();
    if ( m_proxySettings.result() == QDialog::Accepted )
    {
        requiresRestart();
        m_proxySettings.saveSettings();
    }
}


void
SettingsDialog::toggleRemoteMode()
{
    m_advancedWidgetUi->staticHostSettingsButton->setEnabled( m_advancedWidgetUi->staticIpRadioButton->isChecked() );
}


void
SettingsDialog::toggleProxyEnabled()
{
    m_advancedWidgetUi->proxyButton->setEnabled( m_advancedWidgetUi->enableProxyCheckBox->isChecked() );
}


void
SettingsDialog::pickDownloadsPath()
{
    const QString dir = QFileDialog::getExistingDirectory( m_downloadsWidget, tr( "Open Directory" ),
                                                     QDir::homePath(),
                                                     QFileDialog::ShowDirsOnly );

    if ( !dir.isEmpty() )
    {
        m_downloadsWidgetUi->downloadsFolder->setText( dir );
    }
}


void
SettingsDialog::addLibraryPath()
{
    const QString dir = QFileDialog::getExistingDirectory( m_collectionWidget, tr( "Open Directory" ),
                                                     QDir::homePath(),
                                                     QFileDialog::ShowDirsOnly );

    if ( !dir.isEmpty() )
    {
        m_collectionWidgetUi->pathListWidget->addItem( dir );
    }
}


void
SettingsDialog::removeLibraryPath()
{
    if ( m_collectionWidgetUi->pathListWidget->currentRow() >= 0 )
    {
        m_collectionWidgetUi->pathListWidget->takeItem( m_collectionWidgetUi->pathListWidget->currentRow() );
    }
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
    QPointer< AccountFactoryWrapper > watcher( &dialog );

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
    const QString resolver = QFileDialog::getOpenFileName( m_accountsWidget, tr( "Install plug-in from file" ),
                                                           TomahawkSettings::instance()->scriptDefaultPath(),
                                                           tr( "%applicationName Plug-Ins (*.axe *.js);;"
                                                           "All files (*)" ),
                                                           0,
                                                           QFileDialog::ReadOnly );

    if ( !resolver.isEmpty() )
    {
        GlobalActionManager::instance()->installResolverFromFile( resolver );
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


HostDialog::HostDialog( QWidget* parent )
    : QDialog( parent )
    , ui( new Ui::HostDialog )
{
    ui->setupUi( this );

    TomahawkSettings* s = TomahawkSettings::instance();

    connect( ui->autoDetectIpCheckBox, SIGNAL( toggled( bool ) ),
             SLOT( toggleAutoDetectIp( bool ) ) );

    ui->staticHostName->setText( s->externalHostname() );
    ui->staticPort->setValue( s->externalPort() );
    ui->autoDetectIpCheckBox->setChecked( s->autoDetectExternalIp() );
}


void
HostDialog::saveSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();

    s->setAutoDetectExternalIp( ui->autoDetectIpCheckBox->isChecked() );
    s->setExternalHostname( ui->staticHostName->text() );
    s->setExternalPort( ui->staticPort->value() );

    s->sync();
}


void
HostDialog::toggleAutoDetectIp( bool checked )
{
    ui->staticHostName->setEnabled( !checked );
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
