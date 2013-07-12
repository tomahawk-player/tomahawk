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

#pragma once
#ifndef DATABASE_H
#define DATABASE_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <QMutex>
#include <QVariant>


namespace Tomahawk
{

class DatabaseImpl;
class DatabaseCommand;
class DatabaseWorkerThread;
class DatabaseWorker;
class IdThreadWorker;

class DLLEXPORT DatabaseCommandFactory : public QObject
{
Q_OBJECT
    friend class Database;

public:
    virtual ~DatabaseCommandFactory() {}
    dbcmd_ptr newInstance();

signals:
    void created( const Tomahawk::dbcmd_ptr& command );

protected:
    void notifyCreated( const Tomahawk::dbcmd_ptr& command );

    virtual DatabaseCommand* create() const = 0;
};

template <class COMMAND>
class DatabaseCommandFactoryImplementation : public DatabaseCommandFactory
{
protected:
    virtual COMMAND* create() const { return new COMMAND(); }
};

/*
    This class is really a firewall/pimpl - the public functions of LibraryImpl
    are the ones that operate on the database, without any locks.

    HOWEVER, we're using the command pattern to serialize access to the database
    and provide an async api. You create a DatabaseCommand object, and add it to
    the queue of work. There is a threadpool responsible for exec'ing all
    the non-mutating (readonly) commands and one separate thread for mutating ones,
    so sqlite doesn't write to the Database from multiple threads.
*/
class DLLEXPORT Database : public QObject
{
Q_OBJECT

public:
    static Database* instance();

    explicit Database( const QString& dbname, QObject* parent = 0 );
    ~Database();

    void loadIndex();
    bool isReady() const { return m_ready; }

    DatabaseImpl* impl();

    dbcmd_ptr createCommandInstance( const QVariant& op, const Tomahawk::source_ptr& source );

    // Template implementations need to stay in header!
    template<typename T> void registerCommand()
    {
        registerCommand( new DatabaseCommandFactoryImplementation<T>() );
    }

    template<typename T> DatabaseCommandFactory* commandFactory()
    {
        return commandFactoryByClassName( T::staticMetaObject.className() );
    }

signals:
    void indexReady(); // search index
    void ready();

    void newJobRO( Tomahawk::dbcmd_ptr );
    void newJobRW( Tomahawk::dbcmd_ptr );

public slots:
    void enqueue( const Tomahawk::dbcmd_ptr& lc );
    void enqueue( const QList< Tomahawk::dbcmd_ptr >& lc );

private slots:
    void markAsReady();

private:
    void registerCommand( DatabaseCommandFactory* commandFactory );
    DatabaseCommandFactory* commandFactoryByClassName( const QString& className );
    DatabaseCommandFactory* commandFactoryByCommandName( const QString& commandName );
    dbcmd_ptr createCommandInstance( const QString& commandName );

    bool m_ready;

    DatabaseImpl* m_impl;
    QPointer< DatabaseWorkerThread > m_workerRW;
    QList< QPointer< DatabaseWorkerThread > > m_workerThreads;
    IdThreadWorker* m_idWorker;
    int m_maxConcurrentThreads;

    QHash< QString, DatabaseCommandFactory* > m_commandFactories;
    QHash< QString, QString> m_commandNameClassNameMapping;

    QHash< QThread*, DatabaseImpl* > m_implHash;
    QMutex m_mutex;

    static Database* s_instance;

    friend class Tomahawk::Artist;
    friend class Tomahawk::Album;
};

}

#endif // DATABASE_H
