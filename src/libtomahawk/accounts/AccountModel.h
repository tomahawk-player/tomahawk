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

#include "dllmacro.h"

#include "Account.h"

#include <QAbstractItemModel>


namespace Tomahawk {

namespace Accounts {

class AccountModelNode;

class DLLEXPORT AccountModel : public QAbstractItemModel
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

        CheckboxClickedRole = Qt::UserRole + 29, // the checkbox for this row was toggled
        ButtonClickedRole = Qt::UserRole + 30, // the generic install/create/remove/etc/ button was clicked
    };

    enum RowType {
        TopLevelFactory,
        TopLevelAccount,
        ChildAccount
    };

    enum ItemState {
        Uninstalled = 0, // Attica resolver states
        Installing,
        Installed,
        NeedsUpgrade,
        Upgrading,
        Failed,
        ShippedWithTomahawk, // Built-in account/factory state: Can't uninstall or uninstall, just create
        UniqueFactory // Shipped with tomahawk but is a unique account
    };

    explicit AccountModel( QObject* parent = 0 );

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual QModelIndex parent( const QModelIndex& child ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
    virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

signals:
    void createAccount( Tomahawk::Accounts::AccountFactory* factory );

private slots:
    void accountAdded( Tomahawk::Accounts::Account* );
    void accountRemoved( Tomahawk::Accounts::Account* );
    void accountStateChanged( Account*, Accounts::Account::ConnectionState );

private:
    AccountModelNode* nodeFromIndex( const QModelIndex& index ) const;
    void loadData();

    AccountModelNode* m_rootItem;
};

}

}

#endif // TOMAHAWK_ACCOUNTS_ACCOUNTMODEL_H
