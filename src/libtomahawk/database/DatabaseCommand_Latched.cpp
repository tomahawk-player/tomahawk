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

#include "DatabaseCommand_Latched.h"

DatabaseCommand_Latched::DatabaseCommand_Latched(QObject* parent): DatabaseCommandLoggable(parent)
{

}

DatabaseCommand_Latched::DatabaseCommand_Latched(const Tomahawk::source_ptr& s, QObject* parent): DatabaseCommandLoggable(parent)
{

}

bool DatabaseCommand_Latched::doesMutates() const
{
    return DatabaseCommand::doesMutates();
}

void DatabaseCommand_Latched::exec(DatabaseImpl* )
{
    DatabaseCommand::exec();
}

void DatabaseCommand_Latched::postCommitHook()
{
    DatabaseCommand::postCommitHook();
}

bool DatabaseCommand_Latched::singletonCmd() const
{
    return DatabaseCommand::singletonCmd();
}

bool DatabaseCommand_Latched::localOnly() const
{
    return DatabaseCommand::localOnly();
}

