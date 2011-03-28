/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TOMAHAWK_ACLSYSTEM_H
#define TOMAHAWK_ACLSYSTEM_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QTimer>

#include "dllmacro.h"

class DLLEXPORT ACLSystem : public QObject
{
    Q_OBJECT

    enum ACLType {
        Allow,
        Deny
    };

public:
        
    ACLSystem( QObject *parent = 0 );
    ~ACLSystem();
    
    bool isAuthorized( const QString &dbid, const QString &path );
    void authorize( const QString &dbid, const QString &path, ACLType type );
    
private slots:
    void saveTimerFired();

private:
    QHash< QString, QHash< QString, ACLType> > m_cache;
    QTimer m_saveTimer;
};

#endif // TOMAHAWK_ACLSYSTEM_H
