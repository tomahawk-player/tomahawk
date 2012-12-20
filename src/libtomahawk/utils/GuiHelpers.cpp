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

#include "GuiHelpers.h"

#include <QObject>

#include "accounts/Account.h"
#include "accounts/AccountManager.h"
#include "accounts/DelegateConfigWrapper.h"
#include "TomahawkSettings.h"

namespace TomahawkUtils
{

void
handleAccountAdded( Tomahawk::Accounts::Account* account, bool added )
{
    if ( added )
    {
        account->setEnabled( true );
        account->saveConfig();

        TomahawkSettings::instance()->addAccount( account->accountId() );
        Tomahawk::Accounts::AccountManager::instance()->addAccount( account );
        Tomahawk::Accounts::AccountManager::instance()->hookupAndEnable( account );
    }
    else
    {
        // user pressed cancel
        delete account;
    }
}


class UtilsObject : public QObject
{
    Q_OBJECT
public:
    UtilsObject( DelegateConfigWrapper* w ) : QObject( w ), m_w( w ) {}

public slots:
    void
    accountCreateConfigClosed( int ret )
    {
        Tomahawk::Accounts::Account* account = qobject_cast< Tomahawk::Accounts::Account* >( m_w->property( "accountplugin" ).value< QObject* >() );
        Q_ASSERT( account );

        bool added = ( ret == QDialog::Accepted );

        handleAccountAdded( account, added );
    }

    void
    accountConfigClosed( int ret )
    {
        if( ret == QDialog::Accepted )
        {
            Tomahawk::Accounts::Account* account = qobject_cast< Tomahawk::Accounts::Account* >( m_w->property( "accountplugin" ).value< QObject* >() );
            account->saveConfig();
        }
    }

    void
    accountConfigDelete()
    {
        Tomahawk::Accounts::Account* account = qobject_cast< Tomahawk::Accounts::Account* >( m_w->property( "accountplugin" ).value< QObject* >() );
        Q_ASSERT( account );
        Tomahawk::Accounts::AccountManager::instance()->removeAccount( account );
    }
private:
    DelegateConfigWrapper* m_w;

};


void
createAccountFromFactory( Tomahawk::Accounts::AccountFactory* factory, QWidget* parent )
{
    //if exited with OK, create it, if not, delete it immediately!
    Tomahawk::Accounts::Account* account = factory->createAccount();
    bool added = false;
    if( account->configurationWidget() )
    {
#ifdef Q_WS_MAC
        // on osx a sheet needs to be non-modal
        DelegateConfigWrapper* dialog = new DelegateConfigWrapper( account->configurationWidget(), account->aboutWidget(), QObject::tr( "%1 Config" ).arg( account->accountFriendlyName() ), parent, Qt::Sheet );
        dialog->setProperty( "accountplugin", QVariant::fromValue< QObject* >( account ) );

        UtilsObject* obj = new UtilsObject( dialog );
        QObject::connect( dialog, SIGNAL( finished( int ) ), obj, SLOT( accountCreateConfigClosed( int ) ) );

        if( account->configurationWidget()->metaObject()->indexOfSignal( "dataError(bool)" ) > -1 )
            QObject::connect( account->configurationWidget(), SIGNAL( dataError( bool ) ), dialog, SLOT( toggleOkButton( bool ) ), Qt::UniqueConnection );

        dialog->show();
#else
        DelegateConfigWrapper dialog( account->configurationWidget(), account->aboutWidget(), QObject::tr( "%1 Config" ).arg( account->accountFriendlyName() ), parent );
        QWeakPointer< DelegateConfigWrapper > watcher( &dialog );

        if( account->configurationWidget()->metaObject()->indexOfSignal( "dataError(bool)" ) > -1 )
            QObject::connect( account->configurationWidget(), SIGNAL( dataError( bool ) ), &dialog, SLOT( toggleOkButton( bool ) ), Qt::UniqueConnection );

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
openAccountConfig( Tomahawk::Accounts::Account* account, QWidget* parent, bool showDelete )
{
    if( account->configurationWidget() )
    {
#ifndef Q_OS_MAC
        DelegateConfigWrapper dialog( account->configurationWidget(), account->aboutWidget(), QObject::tr("%1 Configuration" ).arg( account->accountFriendlyName() ), parent );
        dialog.setShowDelete( showDelete );
        QWeakPointer< DelegateConfigWrapper > watcher( &dialog );
        int ret = dialog.exec();
        if ( !watcher.isNull() && dialog.deleted() )
        {
            Tomahawk::Accounts::AccountManager::instance()->removeAccount( account );
        }
        else if( !watcher.isNull() && ret == QDialog::Accepted )
        {
            // send changed config to resolver
            account->saveConfig();
        }
#else
        // on osx a sheet needs to be non-modal
        DelegateConfigWrapper* dialog = new DelegateConfigWrapper( account->configurationWidget(), account->aboutWidget(), QObject::tr("%1 Configuration" ).arg( account->accountFriendlyName() ), parent, Qt::Sheet );
        dialog->setShowDelete( showDelete );
        dialog->setProperty( "accountplugin", QVariant::fromValue< QObject* >( account ) );
        UtilsObject* obj = new UtilsObject( dialog );

        QObject::connect( dialog, SIGNAL( finished( int ) ),    obj, SLOT( accountConfigClosed( int ) ) );
        QObject::connect( dialog, SIGNAL( closedWithDelete() ), obj, SLOT( accountConfigDelete() ) );

        dialog->show();
#endif
    }
}

} // namespace TomahawkUtils

#include "GuiHelpers.moc"
