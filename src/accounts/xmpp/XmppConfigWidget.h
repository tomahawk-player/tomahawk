/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef JABBERACCOUNTCONFIGWIDGET_H
#define JABBERACCOUNTCONFIGWIDGET_H

#include "accounts/AccountDllMacro.h"
#include "accounts/AccountConfigWidget.h"
#include <QWidget>

namespace Ui
{
    class XmppConfigWidget;
}

namespace Tomahawk
{

namespace Accounts
{

class XmppAccount;
class GoogleWrapper;


class ACCOUNTDLLEXPORT XmppConfigWidget : public AccountConfigWidget
{
    Q_OBJECT

public:
    explicit XmppConfigWidget( XmppAccount* account = 0, QWidget *parent = 0 );
    virtual ~XmppConfigWidget();

    void saveConfig();

    virtual void checkForErrors();

signals:
    void dataError( bool exists );

private slots:
    void onCheckJidExists( const QString& jid );

private:
    Ui::XmppConfigWidget *m_ui;
    XmppAccount *m_account;

    bool m_serverWasEditedByUser;

    bool m_disableChecksForGoogle;
    friend class GoogleWrapper; // So google wrapper can modify the labels and text
};

}

}

#endif // TWITTERCONFIGWIDGET_H
