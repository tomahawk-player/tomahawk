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

#ifndef DATABASECOMMAND_LATCHED_H
#define DATABASECOMMAND_LATCHED_H

#include "database/databasecommandloggable.h"

class DatabaseImpl;

class DatabaseCommand_Latched : public DatabaseCommandLoggable
{
    Q_OBJECT
public:
    enum LatchAction {
        LatchedOn = 0,
        LatchedOff
    };

    explicit DatabaseCommand_Latched( QObject* parent = 0 );
    explicit DatabaseCommand_Latched( const Tomahawk::source_ptr& s, QObject* parent = 0 );

    virtual bool doesMutates() const { return true; }
    virtual void exec( DatabaseImpl* );
    virtual void postCommitHook();
    virtual bool singletonCmd() const;
    virtual bool localOnly() const;
};

#endif // DATABASECOMMAND_LATCHED_H
