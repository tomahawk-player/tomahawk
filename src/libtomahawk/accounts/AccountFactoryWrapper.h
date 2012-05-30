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

#ifndef ACCOUNTFACTORYWRAPPER_H
#define ACCOUNTFACTORYWRAPPER_H

#include "DllMacro.h"

#include <QDialog>
#include <QModelIndex>

class QAbstractButton;
namespace Tomahawk {
namespace Accounts {
    class AccountFactory;
class Account;
}
}

class Ui_AccountFactoryWrapper;

// class AccountFactoryWrapper_
class DLLEXPORT AccountFactoryWrapper : public QDialog
{
    Q_OBJECT
public:
    enum ExtraRoles {
        AccountRole = Qt::UserRole + 140
    };

    explicit AccountFactoryWrapper( Tomahawk::Accounts::AccountFactory* factory, QWidget* parent = 0 );
    virtual ~AccountFactoryWrapper() {}

public slots:
    void openAccountConfig( Tomahawk::Accounts::Account* );
    void removeAccount( Tomahawk::Accounts::Account* );
    void accountCheckedOrUnchecked( const QModelIndex& , Tomahawk::Accounts::Account* , Qt::CheckState );

private slots:
    void buttonClicked( QAbstractButton* );
    void load();

private:
    Tomahawk::Accounts::AccountFactory* m_factory;
    Ui_AccountFactoryWrapper* m_ui;
    QPushButton* m_addButton;
};

#endif // ACCOUNTFACTORYWRAPPER_H
