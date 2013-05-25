/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef TOMAHAWK_TESTDATABASE_H
#define TOMAHAWK_TESTDATABASE_H

#include <QtTest>

#include "database/Database.h"
#include "database/DatabaseCommand_LogPlayback.h"


class TestDatabaseCommand : public Tomahawk::DatabaseCommand
{
Q_OBJECT
    virtual QString commandname() const { return "TestCommand"; }
};

class TestDatabase : public QObject
{
    Q_OBJECT

private slots:
    void testFactories()
    {
        Tomahawk::Database* db = new Tomahawk::Database("test");
        Tomahawk::dbcmd_ptr command;

        // can we check that his ASSERTs?, it's a build in type, one must not register it again
        // db->registerCommand<DatabaseCommand_LogPlayback>();

        // check that if we request a factory for LogPlayback it really creates a LogPlayback object
        command = db->commandFactory<Tomahawk::DatabaseCommand_LogPlayback>()->newInstance();
        Tomahawk::DatabaseCommand_LogPlayback* lpCmd =  qobject_cast< Tomahawk::DatabaseCommand_LogPlayback* >( command.data() );
        QVERIFY( lpCmd );

        // try to handle a third party database command

        // test no command factory is available until now
        QVERIFY( !db->commandFactory<TestDatabaseCommand>() );

        // register it
        db->registerCommand<TestDatabaseCommand>();

        // make sure it's available now
        command = db->commandFactory<TestDatabaseCommand>()->newInstance();
        TestDatabaseCommand* tCmd = qobject_cast< TestDatabaseCommand* >( command.data() );
        QVERIFY( tCmd );

        delete db;
    }
};

#endif // TOMAHAWK_TESTDATABASE_H
