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

#ifndef DATABASE_H
#define DATABASE_H

#include <QSharedPointer>
#include <QVariant>

#include "Artist.h"
#include "Album.h"
#include "Source.h"
#include "DatabaseCommand.h"

#include "DllMacro.h"

class DatabaseImpl;
class DatabaseWorkerThread;
class DatabaseWorker;
class IdThreadWorker;

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

signals:
    void indexReady(); // search index
    void ready();

    void newJobRO( QSharedPointer<DatabaseCommand> );
    void newJobRW( QSharedPointer<DatabaseCommand> );

public slots:
    void enqueue( const QSharedPointer<DatabaseCommand>& lc );
    void enqueue( const QList< QSharedPointer<DatabaseCommand> >& lc );

private slots:
    void markAsReady();

private:
    bool m_ready;

    DatabaseImpl* m_impl;
    QPointer< DatabaseWorkerThread > m_workerRW;
    QList< QPointer< DatabaseWorkerThread > > m_workerThreads;
    IdThreadWorker* m_idWorker;
    int m_maxConcurrentThreads;

    QHash< QThread*, DatabaseImpl* > m_implHash;
    QMutex m_mutex;

    static Database* s_instance;

    friend class Tomahawk::Artist;
    friend class Tomahawk::Album;
};

#endif // DATABASE_H
