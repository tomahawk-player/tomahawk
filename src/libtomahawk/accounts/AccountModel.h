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

#ifndef TOMAHAWK_ACCOUNTS_ACCOUNTMODEL_H
#define TOMAHAWK_ACCOUNTS_ACCOUNTMODEL_H

#include "DllMacro.h"

#include "Account.h"

#include <QAbstractListModel>
#include <QSet>


namespace Tomahawk {

namespace Accounts {

struct AccountModelNode;

class DLLEXPORT AccountModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        RowTypeRole = Qt::UserRole + 1, // RowType enum

        // Used by top-level accounts
        DescriptionRole = Qt::UserRole + 17,
        StateRole = Qt::UserRole + 18, // ItemState,
        RatingRole = Qt::UserRole + 19,
        DownloadCounterRole = Qt::UserRole + 20,
        VersionRole = Qt::UserRole + 21,
        AuthorRole = Qt::UserRole + 22,
        UserHasRatedRole = Qt::UserRole + 24,

        // used by both
        ConnectionStateRole = Qt::UserRole + 25,
        HasConfig = Qt::UserRole + 26,
        ErrorString = Qt::UserRole + 27,

        // used by individual accounts
        AccountData = Qt::UserRole + 28, // raw plugin
        CanRateRole = Qt::UserRole + 32,
        AccountTypeRole = Qt::UserRole + 33,
        CanDeleteRole = Qt::UserRole + 34,

        CheckboxClickedRole = Qt::UserRole + 29, // the checkbox for this row was toggled
        CustomButtonRole = Qt::UserRole + 30, // the add account or remove account button

        // Used by factories
        ChildrenOfFactoryRole = Qt::UserRole + 31
    };

    enum RowType {
        TopLevelFactory,
        TopLevelAccount,
        UniqueFactory,
        CustomAccount
    };

    enum ItemState {
        Uninstalled = 0, // Attica resolver states
        Installing,
        Installed,
        NeedsUpgrade,
        Upgrading,
        Failed,
        ShippedWithTomahawk, // Built-in account/factory state: Can't uninstall or uninstall, just create
    };

    explicit AccountModel( QObject* parent = 0 );

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

signals:
    void createAccount( Tomahawk::Accounts::AccountFactory* factory );
    void scrollTo( const QModelIndex& idx );

    void startInstalling( const QPersistentModelIndex& idx );
    void doneInstalling( const QPersistentModelIndex& idx );
    void errorInstalling( const QPersistentModelIndex& idx );

private slots:
    void atticaLoaded();
    void loadData();

    void accountAdded( Tomahawk::Accounts::Account* );
    void accountRemoved( Tomahawk::Accounts::Account* );
    void accountStateChanged( Account*, Accounts::Account::ConnectionState );

    void onStartedInstalling( const QString& resolverId );
    void onFinishedInstalling( const QString& resolverId );
    void resolverInstallFailed( const QString& resolverId );
private:
    QModelIndex indexForAtticaId( const QString& resolverId ) const;

    bool m_waitingForAtticaLoaded;
    QList< AccountModelNode* > m_accounts;
    QSet< QString > m_waitingForAtticaInstall;
};

}

}

#endif // TOMAHAWK_ACCOUNTS_ACCOUNTMODEL_H
