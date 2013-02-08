/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef SCRIPTCOMMANDQUEUE_H
#define SCRIPTCOMMANDQUEUE_H

#include "ScriptCommand.h"

#include <QQueue>
#include <QSharedPointer>
#include <QTimer>
#include <QMetaType>
#include <QMutex>

class ScriptCommandQueue : public QObject
{
    Q_OBJECT
public:
    explicit ScriptCommandQueue( QObject* parent = 0 );
    virtual ~ScriptCommandQueue() {}

    void enqueue( const QSharedPointer< ScriptCommand >& req );

private slots:
    void nextCommand();
    void onCommandDone();
    void onTimeout();

private:
    QQueue< QSharedPointer< ScriptCommand > > m_queue;
    QTimer* m_timer;
    QMutex m_mutex;
};

Q_DECLARE_METATYPE( QSharedPointer< ScriptCommand > )

#endif // SCRIPTCOMMANDQUEUE_H
