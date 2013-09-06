/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#ifndef HATCHET_ACCOUNT_CONFIG_H
#define HATCHET_ACCOUNT_CONFIG_H

#include <accounts/AccountConfigWidget.h>

#include <QWidget>
#include <QVariantMap>

class QNetworkReply;

namespace Ui {
    class HatchetAccountConfig;
}

namespace Tomahawk {
namespace Accounts {

class HatchetAccount;

class HatchetAccountConfig : public AccountConfigWidget
{
    Q_OBJECT
public:
    explicit HatchetAccountConfig( HatchetAccount* account );
    virtual ~HatchetAccountConfig();

private slots:
    void login();

    void fieldsChanged();

    void showLoggedIn();
    void showLoggedOut();

    void accountInfoUpdated();

    void authError( const QString& error, int statusCode, const QVariantMap& resp );

protected:
    //virtual void changeEvent( QEvent* event );
    virtual void showEvent( QShowEvent* event );

private:
    Ui::HatchetAccountConfig* m_ui;
    HatchetAccount* m_account;
};

}
}

#endif
