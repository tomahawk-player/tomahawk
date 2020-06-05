/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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
#include "DelegateConfigWrapper.h"

#include "Account.h"
#include "AccountConfigWidget.h"
#include "../utils/Logger.h"

#include <QMessageBox>
#include <QLabel>


DelegateConfigWrapper::DelegateConfigWrapper( Tomahawk::Accounts::Account* account, QWidget* parent, Qt::WindowFlags flags )
    : QDialog( parent, flags )
    , m_account( account )
    , m_widget( account->configurationWidget() )
    , m_aboutW( account->aboutWidget() )
    , m_buttons( nullptr )
    , m_okButton( nullptr )
    , m_deleteButton( nullptr )
    , m_errorLabel( new QLabel( this ) )
    , m_deleted( false )
{
    setWindowTitle(  tr("%1 Config", "Window title for account config windows" ).arg( account->accountFriendlyName() ) );
    QVBoxLayout* v = new QVBoxLayout( this );
    v->setContentsMargins( 0, 0, 0, 0 );
    v->addWidget( m_widget );
    v->addStretch();

    QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    if ( m_aboutW )
    {
        m_aboutW->hide();
        buttons |= QDialogButtonBox::Help;
    }

    m_buttons = new QDialogButtonBox( buttons, Qt::Horizontal, this );
    m_okButton = m_buttons->button( QDialogButtonBox::Ok );
    connect( m_buttons, SIGNAL( clicked( QAbstractButton*)  ), this, SLOT( closed( QAbstractButton* ) ) );
    connect( this, SIGNAL( rejected() ), this, SLOT( rejected() ) );

    if ( m_aboutW )
    {
        connect( m_buttons->button( QDialogButtonBox::Help ), SIGNAL( clicked( bool ) ), this, SLOT( aboutClicked( bool ) ) );
        m_buttons->button( QDialogButtonBox::Help )->setText( tr( "About" ) );
    }

    QHBoxLayout* h = new QHBoxLayout( this );
    h->addWidget( m_buttons );
    if ( m_widget && m_widget->layout() )
        h->setContentsMargins( m_widget->layout()->contentsMargins() );
    else if ( m_widget )
        h->setContentsMargins( m_widget->contentsMargins() );

    m_errorLabel->setAlignment( Qt::AlignCenter );
    m_errorLabel->setWordWrap( true );
    v->addWidget( m_errorLabel );

    v->addLayout( h );
    setLayout( v );

    m_widget->setVisible( true );

    setSizeGripEnabled( false );
    updateSizeHint();

    if ( m_widget->metaObject()->indexOfSignal( "sizeHintChanged()" ) > -1 )
        connect( m_widget, SIGNAL( sizeHintChanged() ), this, SLOT( updateSizeHint() ) );

    connect( m_account, SIGNAL( configTestResult( int, const QString& ) ), SLOT( onConfigTestResult( int, const QString& ) ) );
}


void
DelegateConfigWrapper::setShowDelete( bool del )
{
    if ( del )
        m_deleteButton = m_buttons->addButton( tr( "Delete Account" ), QDialogButtonBox::DestructiveRole );
}


void
DelegateConfigWrapper::toggleOkButton( bool dataError )
{
    // if dataError is True we want to set the button enabled to false
    m_okButton->setEnabled( !dataError );
}


void
DelegateConfigWrapper::closed( QAbstractButton* b )
{
    QDialogButtonBox* buttons = qobject_cast< QDialogButtonBox* >( sender() );

    if ( buttons->standardButton( b ) == QDialogButtonBox::Help )
        return;

    if ( buttons->standardButton( b ) == QDialogButtonBox::Ok )
    {
        // TODO: probably should be hidden behind testConfig() in cpp accounts
        m_widget->resetErrors();
        m_widget->checkForErrors();
        if( !m_widget->settingsValid() )
        {
            foreach( const QString& error, m_widget->errors() )
            {
                QMessageBox::warning( this, tr( "Config Error" ) , error );
            }

            return;
        }

        m_errorLabel->setText( QString() );
        m_account->testConfig();
    }
    else if ( b == m_deleteButton )
    {
        m_deleted = true;
        emit closedWithDelete();
        reject();
        return;
    }
    else
    {
        m_widget->fillDataInWidgets( m_initialData );
        closeDialog( QDialog::Rejected );
    }
}


void
DelegateConfigWrapper::rejected()
{
    layout()->removeWidget( m_widget );
    m_widget->setParent( 0 );
    m_widget->setVisible( false );
}


void
DelegateConfigWrapper::updateSizeHint()
{
    setFixedSize( sizeHint() );
}


void DelegateConfigWrapper::showEvent( QShowEvent* event )
{
    // TODO: would be cool to load the data only on show event instead of on widget creation
    m_initialData = m_widget->readData();

    QDialog::showEvent(event);
}


void
DelegateConfigWrapper::aboutClicked( bool )
{
    Q_ASSERT( m_aboutW );
    m_aboutW->show();

    QDialog d( this );
    d.setWindowTitle( tr( "About this Account" ) );
    QVBoxLayout* v = new QVBoxLayout( &d );
    v->addWidget( m_aboutW );
    QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok, Qt::Horizontal, &d );
    v->addWidget( bbox );

    d.setLayout( v );
    connect( bbox, SIGNAL( clicked( QAbstractButton* ) ), &d, SLOT( accept() ) );
    d.exec();
    v->removeWidget( m_aboutW );

    m_aboutW->setParent( 0 );
    m_aboutW->hide();

}


void
DelegateConfigWrapper::onConfigTestResult( int code, const QString& message )
{
    tLog() << Q_FUNC_INFO << code << ": " << message;

    if( code == Tomahawk::Accounts::ConfigTestResultSuccess )
    {
        m_invalidData = QVariantMap();
        closeDialog( QDialog::Accepted );
    }
    else
    {
        const QVariantMap newData = m_widget->readData();

        // check if user tried to save the same config for the second time
        bool configChangedSinceLastTry = false;
        if ( !m_invalidData.isEmpty() )
        {
            foreach( const QString& key, m_invalidData.keys() )
            {
                if ( m_invalidData[ key ] != newData[ key ] )
                {
                    configChangedSinceLastTry = true;
                    break;
                }
            }

            if ( !configChangedSinceLastTry )
            {
                closeDialog( QDialog::Accepted );
            }
        }

        m_invalidData = m_widget->readData();

	QString msg = !message.isEmpty() ? message : getTestConfigMessage( code );
        m_errorLabel->setText( QString( "<font color='red'>%1</font>" ).arg( msg ) );
    }
}

QString
DelegateConfigWrapper::getTestConfigMessage( int code )
{
    switch(code) {
	case Tomahawk::Accounts::ConfigTestResultCommunicationError:
	    return tr( "Unable to authenticate. Please check your connection." );
	case Tomahawk::Accounts::ConfigTestResultInvalidCredentials:
	    return tr( "Username or password incorrect." );
	case Tomahawk::Accounts::ConfigTestResultInvalidAccount:
	    return tr( "Account rejected by server." );
	case Tomahawk::Accounts::ConfigTestResultPlayingElsewhere:
	    return tr( "Action not allowed, account is in use elsewhere." );
	case Tomahawk::Accounts::ConfigTestResultAccountExpired:
	    return tr( "Your account has expired." );
    }
    return QString();
}


void
DelegateConfigWrapper::closeDialog( QDialog::DialogCode code )
{
    // let the config widget live to see another day
    layout()->removeWidget( m_widget );
    m_widget->setParent( 0 );
    m_widget->setVisible( false );
    m_errorLabel->setText( "" );

    done( code );
}
