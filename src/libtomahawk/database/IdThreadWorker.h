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
#ifndef IDTHREADWORKER_H
#define IDTHREADWORKER_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <QMutex>

struct QueueItem;
class Database;
class DatabaseImpl;

class DLLEXPORT IdThreadWorker : public QThread
{
    Q_OBJECT
public:
    explicit IdThreadWorker( Database* db );
    virtual ~IdThreadWorker();

    void run();
    void stop();

    static void getArtistId( const Tomahawk::artist_ptr& artist, bool autoCreate = false );
    static void getAlbumId( const Tomahawk::album_ptr& album, bool autoCreate = false );

private:
    Database* m_db;
    DatabaseImpl* m_impl;
    bool m_stop;

    static QQueue< QueueItem* > s_workQueue;
};

#endif // IDTHREADWORKER_H
