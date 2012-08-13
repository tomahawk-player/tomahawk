/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#ifndef ACCOUNTSTOOLBUTTON_H
#define ACCOUNTSTOOLBUTTON_H

#include "AccountsPopupWidget.h"

#include "accounts/AccountModel.h"
#include "AccountModelFactoryProxy.h"
#include "DllMacro.h"
#include "Typedefs.h"

#include <QToolButton>

class DLLEXPORT AccountsToolButton : public QToolButton
{
    Q_OBJECT
public:
    explicit AccountsToolButton( QWidget* parent = 0 );

protected:
    void mousePressEvent( QMouseEvent *event );

private slots:
    void popupHidden();

private:
    AccountsPopupWidget* m_popup;
    Tomahawk::Accounts::AccountModel *m_model;
    AccountModelFactoryProxy *m_proxy;
};

#endif // ACCOUNTSTOOLBUTTON_H
