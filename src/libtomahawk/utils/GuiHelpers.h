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

#ifndef TOMAHAWK_GUI_HELPERS_H
#define TOMAHAWK_GUI_HELPERS_H

#include "DllMacro.h"

class QWidget;
namespace Tomahawk {
    namespace Accounts {
        class AccountFactory;
        class Account;
    }
}

namespace TomahawkUtils
{
    DLLEXPORT void createAccountFromFactory( Tomahawk::Accounts::AccountFactory*, QWidget* parent );
    DLLEXPORT void openAccountConfig( Tomahawk::Accounts::Account*, QWidget* parent, bool showDelete = false );
}

#endif
