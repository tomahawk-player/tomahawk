/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef PLAYDARAPI_H
#define PLAYDARAPI_H

#include "PlaydarAPIDllMacro.h"

#include <QHostAddress>
#include <QObject>

class PlaydarApiPrivate;

class TOMAHAWK_PLAYDARAPI_EXPORT PlaydarApi : public QObject
{
    Q_OBJECT
public:
    explicit PlaydarApi( QHostAddress ha, qint16 port, QObject *parent = 0 );
    virtual ~PlaydarApi();

    void start();
    
protected:
    QScopedPointer<PlaydarApiPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE( PlaydarApi )
};

#endif // PLAYDARAPI_H
