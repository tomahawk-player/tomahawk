/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "settingsdialog.h"
#include "config.h"

#include "utils/tomahawkutilsgui.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkConfiguration>
#include <QNetworkProxy>
#include <QVBoxLayout>
#include <QSizeGrip>

#ifdef LIBLASTFM_FOUND
#include <lastfm/ws.h>
#include <lastfm/XmlQuery>
#endif

#include "AtticaManager.h"
#include "tomahawkapp.h"
#include "tomahawksettings.h"
#include "delegateconfigwrapper.h"
#include "musicscanner.h"
#include "pipeline.h"
#include "resolver.h"
#include "ExternalResolverGui.h"
#include "scanmanager.h"
#include "settingslistdelegate.h"
#include "AccountDelegate.h"
#include "database/database.h"
#include "network/servent.h"
#include "playlist/dynamic/widgets/LoadingSpinner.h"
#include "accounts/AccountModel.h"
#include "accounts/Account.h"
#include "accounts/AccountManager.h"
#include <accounts/AccountModelFilterProxy.h>
#include "utils/logger.h"
#include "AccountFactoryWrapper.h"

#include "ui_proxydialog.h"
#include "ui_stackedsettingsdialog.h"

using namespace Tomahawk;
using namespace Accounts;

SettingsDialog::SettingsDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui_StackedSettingsDialog )
    , m_proxySettings( this )
    , m_rejected( false )
    , m_accountModel( 0 )
    , m_sipSpinner( 0 )
{
    ui->setupUi( this );
    TomahawkSettings* s = TomahawkSettings::instance();

    TomahawkUtils::unmarginLayout( layout() );
    ui->stackedWidget->setContentsMargins( 4, 4, 4, 0 );

    ui->checkBoxReporter->setChecked( s->crashReporterEnabled() );
    ui->checkBoxHttp->setChecked( s->httpEnabled() );
    ui->checkBoxStaticPreferred->setChecked( s->preferStaticHostPort() );
    ui->checkBoxUpnp->setChecked( s->externalAddressMode() == TomahawkSettings::Upnp );
    ui->checkBoxUpnp->setEnabled( !s->preferStaticHostPort() );

    createIcons();
#ifdef Q_WS_X11
    ui->listWidget->setFrameShape( QFrame::StyledPanel );
    ui->listWidget->setFrameShadow( QFrame::Sunken );
    setContentsMargins( 4, 4, 4, 4 );
#else
    setContentsMargins( 0, 4, 4, 4 );
#endif

#ifdef Q_WS_MAC
    ui->listWidget->setFixedWidth( 83 );
#endif

#ifdef Q_WS_MAC
    // Avoid resize handles on sheets on osx
    m_proxySettings.setSizeGripEnabled( true );
    QSizeGrip* p = m_proxySettings.findChild< QSizeGrip* >();
    p->setFixedSize( 0, 0 );
#endif

    // Accounts
    AccountDelegate* accountDelegate = new AccountDelegate( this );
    ui->accountsView->setItemDelegate( accountDelegate );
    ui->accountsView->setContextMenuPolicy( Qt::CustomContextMenu );
    ui->accountsView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    ui->accountsView->setMouseTracking( true );

    connect( accountDelegate, SIGNAL( openConfig( Tomahawk::Accounts::Account* ) ), this, SLOT( openAccountConfig( Tomahawk::Accounts::Account* ) ) );
    connect( accountDelegate, SIGNAL( openConfig( Tomahawk::Accounts::AccountFactory* ) ), this, SLOT( openAccountFactoryConfig( Tomahawk::Accounts::AccountFactory* ) ) );
    connect( accountDelegate, SIGNAL( update( QModelIndex ) ), ui->accountsView, SLOT( update( QModelIndex ) ) );

    m_accountModel = new AccountModel( this );
    m_accountProxy = new AccountModelFilterProxy( m_accountModel );
    m_accountProxy->setSourceModel( m_accountModel );

    ui->accountsView->setModel( m_accountProxy );

    connect( m_accountModel, SIGNAL( createAccount( Tomahawk::Accounts::AccountFactory* ) ), this, SLOT( createAccountFromFactory( Tomahawk::Accounts::AccountFactory* ) ) );

    ui->accountsFilterCombo->addItem( tr( "All" ), Accounts::NoType );
    ui->accountsFilterCombo->addItem( accountTypeToString( SipType ), SipType );
    ui->accountsFilterCombo->addItem( accountTypeToString( ResolverType ), ResolverType );
    ui->accountsFilterCombo->addItem( accountTypeToString( InfoType ), InfoType );

    connect( ui->accountsFilterCombo, SIGNAL( activated( int ) ), this, SLOT( accountsFilterChanged( int ) ) );

    if ( !Servent::instance()->isReady() )
    {
        m_sipSpinner = new LoadingSpinner( ui->accountsView );
        m_sipSpinner->fadeIn();

        connect( Servent::instance(), SIGNAL( ready() ), this, SLOT( serventReady() ) );
    }

    // ADVANCED
    ui->staticHostName->setText( s->externalHostname() );
    ui->staticPort->setValue( s->externalPort() );
    ui->proxyButton->setVisible( true );

    ui->checkBoxWatchForChanges->setChecked( s->watchForChanges() );
    ui->scannerTimeSpinBox->setValue( s->scannerTime() );
    ui->enableEchonestCatalog->setChecked( s->enableEchonestCatalogs() );

    connect( ui->checkBoxWatchForChanges, SIGNAL( clicked( bool ) ), SLOT( updateScanOptionsView() ) );

    if ( ui->checkBoxWatchForChanges->isChecked() )
    {
        ui->scanTimeLabel->show();
        ui->scannerTimeSpinBox->show();
    }
    else
    {
        ui->scanTimeLabel->hide();
        ui->scannerTimeSpinBox->hide();
    }

    foreach ( const QString& dir, TomahawkSettings::instance()->scannerPaths() )
    {
        ui->dirTree->checkPath( dir, Qt::Checked );
    }

    // NOW PLAYING
#ifdef Q_WS_MAC
    ui->checkBoxEnableAdium->setChecked( s->nowPlayingEnabled() );
#else
    ui->checkBoxEnableAdium->hide();
#endif

    // LAST FM
    ui->checkBoxEnableLastfm->setChecked( s->scrobblingEnabled() );
    ui->lineEditLastfmUsername->setText( s->lastFmUsername() );
    ui->lineEditLastfmPassword->setText(s->lastFmPassword() );
    connect( ui->pushButtonTestLastfmLogin, SIGNAL( clicked( bool) ), SLOT( testLastFmLogin() ) );

#ifdef Q_WS_MAC // FIXME
    ui->pushButtonTestLastfmLogin->setVisible( false );
#endif

    connect( ui->proxyButton,  SIGNAL( clicked() ),  SLOT( showProxySettings() ) );
    connect( ui->checkBoxStaticPreferred, SIGNAL( toggled(bool) ), SLOT( toggleUpnp(bool) ) );
    connect( ui->checkBoxStaticPreferred, SIGNAL( toggled(bool) ), SLOT( requiresRestart() ) );
    connect( ui->checkBoxUpnp, SIGNAL( toggled(bool) ), SLOT( requiresRestart() ) );
    connect( ui->checkBoxReporter, SIGNAL( toggled(bool) ), SLOT( requiresRestart() ) );
    connect( this, SIGNAL( rejected() ), SLOT( onRejected() ) );

    ui->listWidget->setCurrentRow( 0 );
    ui->listWidget->setItemDelegate(new SettingsListDelegate());
}


SettingsDialog::~SettingsDialog()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_rejected )
    {
        TomahawkSettings* s = TomahawkSettings::instance();

        s->setCrashReporterEnabled( ui->checkBoxReporter->checkState() == Qt::Checked );
        s->setHttpEnabled( ui->checkBoxHttp->checkState() == Qt::Checked );
        s->setPreferStaticHostPort( ui->checkBoxStaticPreferred->checkState() == Qt::Checked );
        s->setExternalAddressMode( ui->checkBoxUpnp->checkState() == Qt::Checked ? TomahawkSettings::Upnp : TomahawkSettings::Lan );

        s->setExternalHostname( ui->staticHostName->text() );
        s->setExternalPort( ui->staticPort->value() );

        s->setScannerPaths( ui->dirTree->getCheckedPaths() );
        s->setWatchForChanges( ui->checkBoxWatchForChanges->isChecked() );
        s->setScannerTime( ui->scannerTimeSpinBox->value() );
        s->setEnableEchonestCatalogs( ui->enableEchonestCatalog->isChecked() );

        s->setNowPlayingEnabled( ui->checkBoxEnableAdium->isChecked() );

        s->setScrobblingEnabled( ui->checkBoxEnableLastfm->isChecked() );
        s->setLastFmUsername( ui->lineEditLastfmUsername->text() );
        s->setLastFmPassword( ui->lineEditLastfmPassword->text() );

        s->applyChanges();
        s->sync();
    }
    else
        qDebug() << "Settings dialog cancelled, NOT saving prefs.";

    delete ui;
}


void
SettingsDialog::serventReady()
{
    m_sipSpinner->fadeOut();
}


void
SettingsDialog::createIcons()
{
    /// Not fun but QListWidget sucks. Do our max-width calculation manually
    /// so the icons arre lined up.
    // Resolvers is the longest string... in english. fml.

    ensurePolished();

    int maxlen = 0;
    QFontMetrics fm( font() );
    QListWidgetItem *accountsButton = new QListWidgetItem( ui->listWidget );
    accountsButton->setIcon( QIcon( RESPATH "images/account-settings.png" ) );
    accountsButton->setText( tr( "Accounts" ) );
    accountsButton->setTextAlignment( Qt::AlignHCenter );
    accountsButton->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    maxlen = fm.width( accountsButton->text() );

    QListWidgetItem *musicButton = new QListWidgetItem( ui->listWidget );
    musicButton->setIcon( QIcon( RESPATH "images/music-settings.png" ) );
    musicButton->setText( tr( "Collection" ) );
    musicButton->setTextAlignment( Qt::AlignHCenter );
    musicButton->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    maxlen = qMax( fm.width( musicButton->text() ), maxlen );

    QListWidgetItem *lastfmButton = new QListWidgetItem( ui->listWidget );
    lastfmButton->setIcon( QIcon( RESPATH "images/lastfm-settings.png" ) );
    lastfmButton->setText( tr( "Last.fm" ) );
    lastfmButton->setTextAlignment( Qt::AlignHCenter );
    lastfmButton->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    maxlen = qMax( fm.width( lastfmButton->text() ), maxlen );

    QListWidgetItem *advancedButton = new QListWidgetItem( ui->listWidget );
    advancedButton->setIcon( QIcon( RESPATH "images/advanced-settings.png" ) );
    advancedButton->setText( tr( "Advanced" ) );
    advancedButton->setTextAlignment( Qt::AlignHCenter );
    advancedButton->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    maxlen = qMax( fm.width( advancedButton->text() ), maxlen );

    maxlen += 15; // padding
    accountsButton->setSizeHint( QSize( maxlen, 60 ) );
    musicButton->setSizeHint( QSize( maxlen, 60 ) );
    lastfmButton->setSizeHint( QSize( maxlen, 60 ) );
    advancedButton->setSizeHint( QSize( maxlen, 60 ) );

#ifndef Q_WS_MAC
    // doesn't listen to sizehint...
    ui->listWidget->setFixedWidth( maxlen + 8 );
#endif

    connect( ui->listWidget, SIGNAL( currentItemChanged( QListWidgetItem*, QListWidgetItem* ) ), SLOT( changePage( QListWidgetItem*, QListWidgetItem* ) ) );
}


void
SettingsDialog::changePage( QListWidgetItem* current, QListWidgetItem* previous )
{
    if ( !current )
        current = previous;

    ui->stackedWidget->setCurrentIndex( ui->listWidget->row(current) );
}


void
SettingsDialog::onRejected()
{
    m_rejected = true;
}


void
SettingsDialog::changeEvent( QEvent *e )
{
    QDialog::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
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
SettingsDialog::toggleUpnp( bool preferStaticEnabled )
{
    if ( preferStaticEnabled )
        ui->checkBoxUpnp->setEnabled( false );
    else
        ui->checkBoxUpnp->setEnabled( true );
}


void
SettingsDialog::updateScanOptionsView()
{
    if ( ui->checkBoxWatchForChanges->isChecked() )
    {
        ui->scanTimeLabel->show();
        ui->scannerTimeSpinBox->show();
    }
    else
    {
        ui->scanTimeLabel->hide();
        ui->scannerTimeSpinBox->hide();
    }
}


void
SettingsDialog::testLastFmLogin()
{
#ifdef LIBLASTFM_FOUND
    ui->pushButtonTestLastfmLogin->setEnabled( false );
    ui->pushButtonTestLastfmLogin->setText( "Testing..." );

    QString authToken = TomahawkUtils::md5( ( ui->lineEditLastfmUsername->text().toLower() + TomahawkUtils::md5( ui->lineEditLastfmPassword->text().toUtf8() ) ).toUtf8() );

    // now authenticate w/ last.fm and get our session key
    QMap<QString, QString> query;
    query[ "method" ] = "auth.getMobileSession";
    query[ "username" ] =  ui->lineEditLastfmUsername->text().toLower();
    query[ "authToken" ] = authToken;

    // ensure they have up-to-date settings
    lastfm::setNetworkAccessManager( TomahawkUtils::nam() );

    QNetworkReply* authJob = lastfm::ws::post( query );

    connect( authJob, SIGNAL( finished() ), SLOT( onLastFmFinished() ) );
#endif
}


void
SettingsDialog::onLastFmFinished()
{
#ifdef LIBLASTFM_FOUND
    QNetworkReply* authJob = dynamic_cast<QNetworkReply*>( sender() );
    if( !authJob )
    {
        qDebug() << Q_FUNC_INFO << "No auth job returned!";
        return;
    }
    if( authJob->error() == QNetworkReply::NoError )
    {
        lastfm::XmlQuery lfm = lastfm::XmlQuery( authJob->readAll() );

        if( lfm.children( "error" ).size() > 0 )
        {
            qDebug() << "ERROR from last.fm:" << lfm.text();
            ui->pushButtonTestLastfmLogin->setText( tr( "Failed" ) );
            ui->pushButtonTestLastfmLogin->setEnabled( true );
        }
        else
        {
            ui->pushButtonTestLastfmLogin->setText( tr( "Success" ) );
            ui->pushButtonTestLastfmLogin->setEnabled( false );
        }
    }
    else
    {
        switch( authJob->error() )
        {
            case QNetworkReply::ContentOperationNotPermittedError:
            case QNetworkReply::AuthenticationRequiredError:
                ui->pushButtonTestLastfmLogin->setText( tr( "Failed" ) );
                ui->pushButtonTestLastfmLogin->setEnabled( true );
                break;

            default:
                qDebug() << "Couldn't get last.fm auth result";
                ui->pushButtonTestLastfmLogin->setText( tr( "Could not contact server" ) );
                ui->pushButtonTestLastfmLogin->setEnabled( true );
                return;
        }
    }
#endif
}


void
SettingsDialog::accountsFilterChanged( int )
{
    AccountType filter = static_cast< AccountType >( ui->accountsFilterCombo->itemData( ui->accountsFilterCombo->currentIndex() ).toInt() );
    m_accountProxy->setFilterType( filter );
}


void
SettingsDialog::openAccountConfig( Account* account, bool showDelete )
{
    if( account->configurationWidget() )
    {
#ifndef Q_OS_MAC
        DelegateConfigWrapper dialog( account->configurationWidget(), QString("%1 Configuration" ).arg( account->accountFriendlyName() ), this );
        dialog.setShowDelete( showDelete );
        QWeakPointer< DelegateConfigWrapper > watcher( &dialog );
        int ret = dialog.exec();
        if ( !watcher.isNull() && dialog.deleted() )
        {
            AccountManager::instance()->removeAccount( account );
        }
        else if( !watcher.isNull() && ret == QDialog::Accepted )
        {
            // send changed config to resolver
            account->saveConfig();
        }
#else
        // on osx a sheet needs to be non-modal
        DelegateConfigWrapper* dialog = new DelegateConfigWrapper( account->configurationWidget(), QString("%1 Configuration" ).arg( account->accountFriendlyName() ), this, Qt::Sheet );
        dialog->setShowDelete( showDelete );
        dialog->setProperty( "accountplugin", QVariant::fromValue< QObject* >( account ) );
        connect( dialog, SIGNAL( finished( int ) ), this, SLOT( accountConfigClosed( int ) ) );
        connect( dialog, SIGNAL( closedWithDelete() ), this, SLOT( accountConfigDelete() ) );

        dialog->show();
#endif
    }
}


void
SettingsDialog::accountConfigClosed( int value )
{
    if( value == QDialog::Accepted )
    {
        DelegateConfigWrapper* dialog = qobject_cast< DelegateConfigWrapper* >( sender() );
        Account* account = qobject_cast< Account* >( dialog->property( "accountplugin" ).value< QObject* >() );
        account->saveConfig();
    }
}


void
SettingsDialog::accountConfigDelete()
{
    DelegateConfigWrapper* dialog = qobject_cast< DelegateConfigWrapper* >( sender() );
    Account* account = qobject_cast< Account* >( dialog->property( "accountplugin" ).value< QObject* >() );
    Q_ASSERT( account );
    AccountManager::instance()->removeAccount( account );
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
    AccountFactoryWrapper dialog( factory, this );
    QWeakPointer< AccountFactoryWrapper > watcher( &dialog );

    int ret = dialog.exec();
    if ( !watcher.isNull() && dialog.doCreateAccount() )
        createAccountFromFactory( factory );
#else
    // on osx a sheet needs to be non-modal
    AccountFactoryWrapper* dialog = new AccountFactoryWrapper( factory, this );
    connect( dialog, SIGNAL( createAccount( Tomahawk::Accounts::AccountFactory* ) ), this, SLOT( createAccountFromFactory( Tomahawk::Accounts::AccountFactory* ) ) );

    dialog->show();
#endif
}


void
SettingsDialog::createAccountFromFactory( AccountFactory* factory )
{
    //if exited with OK, create it, if not, delete it immediately!
    Account* account = factory->createAccount();
    bool added = false;
    if( account->configurationWidget() )
    {
#ifdef Q_WS_MAC
        // on osx a sheet needs to be non-modal
        DelegateConfigWrapper* dialog = new DelegateConfigWrapper( account->configurationWidget(), QString("%1 Config" ).arg( account->accountFriendlyName() ), this, Qt::Sheet );
        dialog->setProperty( "accountplugin", QVariant::fromValue< QObject* >( account ) );
        connect( dialog, SIGNAL( finished( int ) ), this, SLOT( accountCreateConfigClosed( int ) ) );

        if( account->configurationWidget()->metaObject()->indexOfSignal( "dataError(bool)" ) > -1 )
            connect( account->configurationWidget(), SIGNAL( dataError( bool ) ), dialog, SLOT( toggleOkButton( bool ) ), Qt::UniqueConnection );

        dialog->show();
#else
        DelegateConfigWrapper dialog( account->configurationWidget(), QString("%1 Config" ).arg( account->accountFriendlyName() ), this );
        QWeakPointer< DelegateConfigWrapper > watcher( &dialog );

        if( account->configurationWidget()->metaObject()->indexOfSignal( "dataError(bool)" ) > -1 )
            connect( account->configurationWidget(), SIGNAL( dataError( bool ) ), &dialog, SLOT( toggleOkButton( bool ) ), Qt::UniqueConnection );

        int ret = dialog.exec();
        if( !watcher.isNull() && ret == QDialog::Accepted ) // send changed config to account
            added = true;
        else // canceled, delete it
            added = false;

        handleAccountAdded( account, added );
#endif
    }
    else
    {
        // no config, so just add it
        added = true;
        handleAccountAdded( account, added );
    }
}


void
SettingsDialog::accountCreateConfigClosed( int finished )
{
    DelegateConfigWrapper* dialog = qobject_cast< DelegateConfigWrapper* >( sender() );
    Account* account = qobject_cast< Account* >( dialog->property( "accountplugin" ).value< QObject* >() );
    Q_ASSERT( account );

    bool added = ( finished == QDialog::Accepted );

    handleAccountAdded( account, added );
}


void
SettingsDialog::handleAccountAdded( Account* account, bool added )
{
    if ( added )
    {
        account->setEnabled( true );
        account->setAutoConnect( true );
        account->saveConfig();

        TomahawkSettings::instance()->addAccount( account->accountId() );
        AccountManager::instance()->addAccount( account );
        AccountManager::instance()->hookupAndEnable( account );
    }
    else
    {
        // user pressed cancel
        delete account;
    }
}


void
SettingsDialog::requiresRestart()
{
    QMessageBox::information( this, tr( "Information" ), tr( "Changing this setting requires a restart of Tomahawk!" ) );
}


ProxyDialog::ProxyDialog( QWidget *parent )
: QDialog( parent )
, ui( new Ui::ProxyDialog )
{
    ui->setupUi( this );

    // ugly, I know, but...

    int i = 0;
    ui->typeBox->insertItem( i, "No Proxy", QNetworkProxy::NoProxy );
    m_forwardMap[ QNetworkProxy::NoProxy ] = i;
    m_backwardMap[ i ] = QNetworkProxy::NoProxy;
    i++;
    ui->typeBox->insertItem( i, "SOCKS 5", QNetworkProxy::Socks5Proxy );
    m_forwardMap[ QNetworkProxy::Socks5Proxy ] = i;
    m_backwardMap[ i ] = QNetworkProxy::Socks5Proxy;
    i++;

    TomahawkSettings* s = TomahawkSettings::instance();

    ui->typeBox->setCurrentIndex( m_forwardMap[s->proxyType()] );
    ui->hostLineEdit->setText( s->proxyHost() );
    ui->portSpinBox->setValue( s->proxyPort() );
    ui->userLineEdit->setText( s->proxyUsername() );
    ui->passwordLineEdit->setText( s->proxyPassword() );
    ui->checkBoxUseProxyForDns->setChecked( s->proxyDns() );
    ui->noHostLineEdit->setText( s->proxyNoProxyHosts() );

    if ( s->proxyType() == QNetworkProxy::NoProxy )
    {
        ui->hostLineEdit->setEnabled( false );
        ui->portSpinBox->setEnabled( false );
        ui->userLineEdit->setEnabled( false );
        ui->passwordLineEdit->setEnabled( false );
        ui->checkBoxUseProxyForDns->setEnabled( false );
        ui->noHostLineEdit->setEnabled( false );
    }

    connect( ui->typeBox, SIGNAL( currentIndexChanged( int ) ), SLOT( proxyTypeChangedSlot( int ) ) );
}


void
ProxyDialog::proxyTypeChangedSlot( int index )
{
    if ( m_backwardMap[ index ] == QNetworkProxy::NoProxy )
    {
        ui->hostLineEdit->setEnabled( false );
        ui->portSpinBox->setEnabled( false );
        ui->userLineEdit->setEnabled( false );
        ui->passwordLineEdit->setEnabled( false );
        ui->checkBoxUseProxyForDns->setEnabled( false );
        ui->noHostLineEdit->setEnabled( false );
    }
    else
    {
        ui->hostLineEdit->setEnabled( true );
        ui->portSpinBox->setEnabled( true );
        ui->userLineEdit->setEnabled( true );
        ui->passwordLineEdit->setEnabled( true );
        ui->checkBoxUseProxyForDns->setEnabled( true );
        ui->noHostLineEdit->setEnabled( true );
    }
}


void
ProxyDialog::saveSettings()
{
    qDebug() << Q_FUNC_INFO;

    QNetworkProxy::ProxyType type = static_cast< QNetworkProxy::ProxyType>( m_backwardMap[ ui->typeBox->currentIndex() ] );

    //First set settings
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setProxyHost( ui->hostLineEdit->text() );

    int port = ui->portSpinBox->value();
    s->setProxyPort( port );
    s->setProxyNoProxyHosts( ui->noHostLineEdit->text() );
    s->setProxyUsername( ui->userLineEdit->text() );
    s->setProxyPassword( ui->passwordLineEdit->text() );
    s->setProxyType( type );
    s->setProxyDns( ui->checkBoxUseProxyForDns->checkState() == Qt::Checked );
    s->sync();

    TomahawkUtils::NetworkProxyFactory* proxyFactory = TomahawkUtils::proxyFactory();
    tDebug() << Q_FUNC_INFO << "Got proxyFactory: " << proxyFactory;
    if ( type == QNetworkProxy::NoProxy )
    {
        tDebug() << Q_FUNC_INFO << "Got NoProxy selected";
        proxyFactory->setProxy( QNetworkProxy::NoProxy );
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "Got Socks5Proxy selected";
        proxyFactory->setProxy( QNetworkProxy( type, s->proxyHost(), s->proxyPort(), s->proxyUsername(), s->proxyPassword() ) );
        if ( !ui->noHostLineEdit->text().isEmpty() )
        {
            tDebug() << Q_FUNC_INFO << "hosts line edit is " << ui->noHostLineEdit->text();
            tDebug() << Q_FUNC_INFO << "split hosts line edit is " << ui->noHostLineEdit->text().split( ' ', QString::SkipEmptyParts );
            proxyFactory->setNoProxyHosts( ui->noHostLineEdit->text().split( ' ', QString::SkipEmptyParts ) );
        }
    }
}
