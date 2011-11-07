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

#include <QCryptographicHash>
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
#include "GetNewStuffDialog.h"
#include "musicscanner.h"
#include "pipeline.h"
#include "resolver.h"
#include "resolverconfigdelegate.h"
#include "resolversmodel.h"
#include "scanmanager.h"
#include "settingslistdelegate.h"
#include "sipconfigdelegate.h"
#include "database/database.h"
#include "network/servent.h"
#include "playlist/dynamic/widgets/LoadingSpinner.h"
#include "sip/SipHandler.h"
#include "sip/SipModel.h"
#include "utils/logger.h"

#include "ui_proxydialog.h"
#include "ui_stackedsettingsdialog.h"

static QString
md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}

SettingsDialog::SettingsDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui_StackedSettingsDialog )
    , m_proxySettings( this )
    , m_rejected( false )
    , m_sipModel( 0 )
    , m_resolversModel( 0 )
    , m_sipSpinner( 0 )
{
    ui->setupUi( this );
    TomahawkSettings* s = TomahawkSettings::instance();

    TomahawkUtils::unmarginLayout( layout() );
    ui->stackedWidget->setContentsMargins( 4, 4, 4, 0 );

    ui->addSipButton->setFixedWidth( 42 );
    ui->removeSipButton->setFixedWidth( ui->addSipButton->width() );
    ui->addScript->setFixedWidth( 42 );
    ui->removeScript->setFixedWidth( ui->addScript->width() );

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
    ui->listWidget->setFixedWidth( 83 );
    setContentsMargins( 0, 4, 4, 4 );
#endif

#ifdef Q_WS_MAC
    // Avoid resize handles on sheets on osx
    m_proxySettings.setSizeGripEnabled( true );
    QSizeGrip* p = m_proxySettings.findChild< QSizeGrip* >();
    p->setFixedSize( 0, 0 );
#endif

    // SIP PLUGINS
    SipConfigDelegate* sipdel = new SipConfigDelegate( this );
    ui->accountsView->setItemDelegate( sipdel );
    ui->accountsView->setContextMenuPolicy( Qt::CustomContextMenu );
    ui->accountsView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

    connect( ui->accountsView, SIGNAL( clicked( QModelIndex ) ), this, SLOT( sipItemClicked( QModelIndex ) ) );
    connect( sipdel, SIGNAL( openConfig( SipPlugin* ) ), this, SLOT( openSipConfig( SipPlugin* ) ) );
    connect( ui->accountsView, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( sipContextMenuRequest( QPoint ) ) );
    m_sipModel = new SipModel( this );
    ui->accountsView->setModel( m_sipModel );

    if ( !Servent::instance()->isReady() )
    {
        m_sipSpinner = new LoadingSpinner( ui->accountsView );
        m_sipSpinner->fadeIn();

        ui->addSipButton->setEnabled( false );
        ui->removeSipButton->setEnabled( false );
        connect( Servent::instance(), SIGNAL( ready() ), this, SLOT( serventReady() ) );
    }

    setupSipButtons();

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

    // SCRIPT RESOLVER
    ui->removeScript->setEnabled( false );
    ResolverConfigDelegate* del = new ResolverConfigDelegate( this );
    connect( del, SIGNAL( openConfig( QString ) ), SLOT( openResolverConfig( QString ) ) );
    ui->scriptList->setItemDelegate( del );
    m_resolversModel = new ResolversModel( this );
    ui->scriptList->setModel( m_resolversModel );
    ui->scriptList->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    connect( m_resolversModel, SIGNAL( openConfig( QString ) ), SLOT( openResolverConfig( QString ) ) );

#ifdef LIBATTICA_FOUND
    connect( ui->getMoreResolvers, SIGNAL( clicked() ), this, SLOT( getMoreResolvers() ) );

    connect( AtticaManager::instance(), SIGNAL( resolverInstalled( QString ) ), this, SLOT( atticaResolverInstalled( QString ) ) );
    connect( AtticaManager::instance(), SIGNAL( resolverUninstalled( QString ) ), this, SLOT( atticaResolverUninstalled( QString ) ) );
#else
    ui->getMoreResolvers->hide();
#endif

    connect( ui->scriptList->selectionModel(), SIGNAL( selectionChanged( QItemSelection,QItemSelection ) ), this, SLOT( scriptSelectionChanged() ) );
    connect( ui->addScript, SIGNAL( clicked( bool ) ), this, SLOT( addScriptResolver() ) );
    connect( ui->removeScript, SIGNAL( clicked( bool ) ), this, SLOT( removeScriptResolver() ) );
    connect( ui->proxyButton,  SIGNAL( clicked() ),  SLOT( showProxySettings() ) );
    connect( ui->checkBoxStaticPreferred, SIGNAL( toggled(bool) ), SLOT( toggleUpnp(bool) ) );
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

        m_resolversModel->saveScriptResolvers();

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
    ui->addSipButton->setEnabled( true );
    ui->removeSipButton->setEnabled( true );
}


void
SettingsDialog::createIcons()
{
    /// Not fun but QListWidget sucks. Do our max-width calculation manually
    /// so the icons arre lined up.
    // Resolvers is the longest string... in english. fml.

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

    QListWidgetItem *resolversButton = new QListWidgetItem( ui->listWidget );
    resolversButton->setIcon( QIcon( RESPATH "images/resolvers-settings.png" ) );
    resolversButton->setText( tr( "Resolvers" ) );
    resolversButton->setTextAlignment( Qt::AlignHCenter );
    resolversButton->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    maxlen = qMax( fm.width( resolversButton->text() ), maxlen );

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
    resolversButton->setSizeHint( QSize( maxlen, 60 ) );
    advancedButton->setSizeHint( QSize( maxlen, 60 ) );

#ifndef Q_WS_MAC
    // doesn't listen to sizehint...
    ui->listWidget->setMaximumWidth( maxlen + 8 );
#endif

    connect( ui->listWidget, SIGNAL( currentItemChanged( QListWidgetItem*, QListWidgetItem* ) ), SLOT( changePage( QListWidgetItem*, QListWidgetItem* ) ) );
}


void
SettingsDialog::setupSipButtons()
{
    foreach( SipPluginFactory* f, SipHandler::instance()->pluginFactories() )
    {
        if( f->isUnique() && SipHandler::instance()->hasPluginType( f->factoryId() ) )
        {
            continue;
        }

        QAction* action = new QAction( f->icon(), f->prettyName(), ui->addSipButton );
        action->setProperty( "factory", QVariant::fromValue< QObject* >( f ) );
        ui->addSipButton->addAction( action );

        connect( action, SIGNAL( triggered(bool) ), this, SLOT( factoryActionTriggered( bool ) ) );
    }

    connect( ui->removeSipButton, SIGNAL( clicked( bool ) ), this, SLOT( sipPluginDeleted( bool ) ) );
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

    QString authToken = md5( ( ui->lineEditLastfmUsername->text().toLower() + md5( ui->lineEditLastfmPassword->text().toUtf8() ) ).toUtf8() );

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
SettingsDialog::addScriptResolver()
{
    QString resolver = QFileDialog::getOpenFileName( this, tr( "Load script resolver file" ), TomahawkSettings::instance()->scriptDefaultPath() );
    if( !resolver.isEmpty() )
    {
        m_resolversModel->addResolver( resolver, true );

        QFileInfo resolverAbsoluteFilePath = resolver;
        TomahawkSettings::instance()->setScriptDefaultPath( resolverAbsoluteFilePath.absolutePath() );
    }
}


void
SettingsDialog::removeScriptResolver()
{
    // only one selection
    if( !ui->scriptList->selectionModel()->selectedIndexes().isEmpty() )
    {
        QString resolver = ui->scriptList->selectionModel()->selectedIndexes().first().data( ResolversModel::ResolverPath ).toString();
#ifdef LIBATTICA_FOUND
        AtticaManager::instance()->uninstallResolver( resolver );
#endif
        m_resolversModel->removeResolver( resolver );
    }
}

void
SettingsDialog::getMoreResolvers()
{
#if defined(Q_WS_MAC) && defined(LIBATTICA_FOUND)
    GetNewStuffDialog* diag = new GetNewStuffDialog( this, Qt::Sheet );
    connect( diag, SIGNAL( finished( int ) ), this, SLOT( getMoreResolversFinished(int)));

    diag->show();
#elif defined(LIBATTICA_FOUND)
    GetNewStuffDialog diag( this );
    int ret = diag.exec();
    Q_UNUSED( ret );
#endif

}

#ifdef LIBATTICA_FOUND
void
SettingsDialog::atticaResolverInstalled( const QString& resolverId )
{
    m_resolversModel->atticaResolverInstalled( resolverId );
}

void
SettingsDialog::atticaResolverUninstalled ( const QString& resolverId )
{
    m_resolversModel->removeResolver( AtticaManager::instance()->pathFromId( resolverId ) );
}
#endif

void
SettingsDialog::scriptSelectionChanged()
{
    if( !ui->scriptList->selectionModel()->selectedIndexes().isEmpty() )
    {
        ui->removeScript->setEnabled( true );
    }
    else
    {
        ui->removeScript->setEnabled( false );
    }
}

void
SettingsDialog::getMoreResolversFinished( int ret )
{
    Q_UNUSED( ret );
}


void
SettingsDialog::openResolverConfig( const QString& resolver )
{
    Tomahawk::ExternalResolver* r = Tomahawk::Pipeline::instance()->resolverForPath( resolver );
    if( r && r->configUI() )
    {
#ifndef Q_WS_MAC
        DelegateConfigWrapper dialog( r->configUI(), "Resolver Configuration", this );
        QWeakPointer< DelegateConfigWrapper > watcher( &dialog );
        int ret = dialog.exec();
        if( !watcher.isNull() && ret == QDialog::Accepted )
        {
            // send changed config to resolver
            r->saveConfig();
        }
#else
        // on osx a sheet needs to be non-modal
        DelegateConfigWrapper* dialog = new DelegateConfigWrapper( r->configUI(), "Resolver Configuration", this, Qt::Sheet );
        dialog->setProperty( "resolver", QVariant::fromValue< QObject* >( r ) );
        connect( dialog, SIGNAL( finished( int ) ), this, SLOT( resolverConfigClosed( int ) ) );

        dialog->show();
#endif

    }
}


void
SettingsDialog::resolverConfigClosed( int value )
{
    if( value == QDialog::Accepted )
    {
        DelegateConfigWrapper* dialog = qobject_cast< DelegateConfigWrapper* >( sender() );
        Tomahawk::ExternalResolver* r = qobject_cast< Tomahawk::ExternalResolver* >( dialog->property( "resolver" ).value< QObject* >() );
        r->saveConfig();
    }
}


void
SettingsDialog::sipItemClicked( const QModelIndex& item )
{
    if( item.data( SipModel::FactoryRole ).toBool() )
        if( ui->accountsView->isExpanded( item ) )
            ui->accountsView->collapse( item );
        else
            ui->accountsView->expand( item );
    else if( item.data( SipModel::FactoryItemRole ).toBool() )
        sipFactoryClicked( qobject_cast<SipPluginFactory* >( item.data( SipModel::SipPluginFactoryData ).value< QObject* >() ) );
}


void
SettingsDialog::openSipConfig( SipPlugin* p )
{
    if( p->configWidget() )
    {
#ifndef Q_WS_MAC
        DelegateConfigWrapper dialog( p->configWidget(), QString("%1 Configuration" ).arg( p->friendlyName() ), this );
        QWeakPointer< DelegateConfigWrapper > watcher( &dialog );
        int ret = dialog.exec();
        if( !watcher.isNull() && ret == QDialog::Accepted )
        {
            // send changed config to resolver
            p->saveConfig();
        }
#else
        // on osx a sheet needs to be non-modal
        DelegateConfigWrapper* dialog = new DelegateConfigWrapper( p->configWidget(), QString("%1 Configuration" ).arg( p->friendlyName() ), this, Qt::Sheet );
        dialog->setProperty( "sipplugin", QVariant::fromValue< QObject* >( p ) );
        connect( dialog, SIGNAL( finished( int ) ), this, SLOT( sipConfigClosed( int ) ) );

        dialog->show();
#endif
    }
}


void
SettingsDialog::sipConfigClosed( int value )
{
    if( value == QDialog::Accepted )
    {
        DelegateConfigWrapper* dialog = qobject_cast< DelegateConfigWrapper* >( sender() );
        SipPlugin* p = qobject_cast< SipPlugin* >( dialog->property( "sipplugin" ).value< QObject* >() );
        p->saveConfig();
    }
}


void
SettingsDialog::factoryActionTriggered( bool )
{
    Q_ASSERT( sender() && qobject_cast< QAction* >( sender() ) );

    QAction* a = qobject_cast< QAction* >( sender() );
    Q_ASSERT( qobject_cast< SipPluginFactory* >( a->property( "factory" ).value< QObject* >() ) );

    SipPluginFactory* f = qobject_cast< SipPluginFactory* >( a->property( "factory" ).value< QObject* >() );
    sipFactoryClicked( f );
}


void
SettingsDialog::sipFactoryClicked( SipPluginFactory* factory )
{
    //if exited with OK, create it, if not, delete it immediately!
    SipPlugin* p = factory->createPlugin();
    bool added = false;
    if( p->configWidget() )
    {
#ifdef Q_WS_MAC
        // on osx a sheet needs to be non-modal
        DelegateConfigWrapper* dialog = new DelegateConfigWrapper( p->configWidget(), QString("%1 Config" ).arg( p->friendlyName() ), this, Qt::Sheet );
        dialog->setProperty( "sipplugin", QVariant::fromValue< QObject* >( p ) );
        connect( dialog, SIGNAL( finished( int ) ), this, SLOT( sipCreateConfigClosed( int ) ) );
        connect( p, SIGNAL( datatError( bool ) ), dialog, SLOT( toggleOkButton( bool ) ) );

        dialog->show();
#else
        DelegateConfigWrapper dialog( p->configWidget(), QString("%1 Config" ).arg( p->friendlyName() ), this );
        QWeakPointer< DelegateConfigWrapper > watcher( &dialog );
        connect( p, SIGNAL( dataError( bool ) ), &dialog, SLOT( toggleOkButton( bool ) ) );
        int ret = dialog.exec();
        if( !watcher.isNull() && ret == QDialog::Accepted )
        {
            // send changed config to resolver
            p->saveConfig();

            // accepted, so add it to tomahawk
            TomahawkSettings::instance()->addSipPlugin( p->pluginId() );
            SipHandler::instance()->addSipPlugin( p );

            added = true;
        }
        else
        {
            // canceled, delete it
            added = false;
        }

        handleSipPluginAdded( p, added );
#endif
    } else
    {
        // no config, so just add it
        added = true;
        TomahawkSettings::instance()->addSipPlugin( p->pluginId() );
        SipHandler::instance()->addSipPlugin( p );

        handleSipPluginAdded( p, added );
    }
}


void
SettingsDialog::sipCreateConfigClosed( int finished )
{
    DelegateConfigWrapper* dialog = qobject_cast< DelegateConfigWrapper* >( sender() );
    SipPlugin* p = qobject_cast< SipPlugin* >( dialog->property( "sipplugin" ).value< QObject* >() );
    Q_ASSERT( p );

    bool added = false;
    if( finished == QDialog::Accepted )
    {

        p->saveConfig();
        TomahawkSettings::instance()->addSipPlugin( p->pluginId() );
        SipHandler::instance()->addSipPlugin( p );

        added = true;
    }

    handleSipPluginAdded( p, added );
}


void
SettingsDialog::handleSipPluginAdded( SipPlugin* p, bool added )
{
    SipPluginFactory* f = SipHandler::instance()->factoryFromPlugin( p );
    if( added && f && f->isUnique() )
    {
        // remove from actions list
        QAction* toremove = 0;
        foreach( QAction* a, ui->addSipButton->actions() )
        {
            if( f == qobject_cast< SipPluginFactory* >( a->property( "factory" ).value< QObject* >() ) )
            {
                toremove = a;
                break;
            }
        }
        if( toremove )
            ui->addSipButton->removeAction( toremove );
    }
    else if( added == false )
    { // user pressed cancel
        delete p;
    }
}


void
SettingsDialog::sipContextMenuRequest( const QPoint& p )
{
    QModelIndex idx = ui->accountsView->indexAt( p );
    // if it's an account, allow to delete
    if( idx.isValid() && !idx.data( SipModel::FactoryRole ).toBool() && !idx.data( SipModel::FactoryItemRole ).toBool() )
    {
        QList< QAction* > acts;
        acts << new QAction( tr( "Delete Account" ), this );
        acts.first()->setProperty( "sipplugin", idx.data( SipModel::SipPluginData ) );
        connect( acts.first(), SIGNAL( triggered( bool ) ), this, SLOT( sipPluginRowDeleted( bool ) ) );
        QMenu::exec( acts, ui->accountsView->mapToGlobal( p ) );
    }
}


void
SettingsDialog::sipPluginRowDeleted( bool )
{
    SipPlugin* p = qobject_cast< SipPlugin* >( qobject_cast< QAction* >( sender() )->property( "sipplugin" ).value< QObject* >() );
    SipHandler::instance()->removeSipPlugin( p );
}


void
SettingsDialog::sipPluginDeleted( bool )
{
    QModelIndexList indexes = ui->accountsView->selectionModel()->selectedIndexes();
    // if it's an account, allow to delete
    foreach( const QModelIndex& idx, indexes )
    {
        if( idx.isValid() && !idx.data( SipModel::FactoryRole ).toBool() && !idx.data( SipModel::FactoryItemRole ).toBool() )
        {
            SipPlugin* p = qobject_cast< SipPlugin* >( idx.data( SipModel::SipPluginData ).value< QObject* >() );

            if( SipPluginFactory* f = SipHandler::instance()->factoryFromPlugin( p ) )
            {
                if( f->isUnique() ) // just deleted a unique plugin->re-add to add menu
                {
                    QAction* action = new QAction( f->icon(), f->prettyName(), ui->addSipButton );
                    action->setProperty( "factory", QVariant::fromValue< QObject* >( f ) );
                    ui->addSipButton->addAction( action );

                    connect( action, SIGNAL( triggered(bool) ), this, SLOT( factoryActionTriggered( bool ) ) );
                }
            }
            SipHandler::instance()->removeSipPlugin( p );
        }
    }
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

    //First set settings
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setProxyHost( ui->hostLineEdit->text() );

    int port = ui->portSpinBox->value();
    s->setProxyPort( port );
    s->setProxyNoProxyHosts( ui->noHostLineEdit->text() );
    s->setProxyUsername( ui->userLineEdit->text() );
    s->setProxyPassword( ui->passwordLineEdit->text() );
    s->setProxyType( m_backwardMap[ ui->typeBox->itemData( ui->typeBox->currentIndex() ).toInt() ] );
    s->setProxyDns( ui->checkBoxUseProxyForDns->checkState() == Qt::Checked );

    if( s->proxyHost().isEmpty() )
        return;

    TomahawkUtils::NetworkProxyFactory* proxyFactory = new TomahawkUtils::NetworkProxyFactory();
    QNetworkProxy proxy( static_cast<QNetworkProxy::ProxyType>(s->proxyType()), s->proxyHost(), s->proxyPort(), s->proxyUsername(), s->proxyPassword() );
    proxyFactory->setProxy( proxy );
    if ( !ui->noHostLineEdit->text().isEmpty() )
        proxyFactory->setNoProxyHosts( ui->noHostLineEdit->text().split( ',', QString::SkipEmptyParts ) );
    TomahawkUtils::setProxyFactory( proxyFactory );
}
