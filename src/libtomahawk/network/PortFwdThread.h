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

#ifndef PORTFWDTHREAD_H
#define PORTFWDTHREAD_H

#include <QThread>
#include <QMutex>
#include <QHostAddress>
#include <QPointer>

class Portfwd;

class PortFwdWorker : public QObject
{
Q_OBJECT

public:
    explicit PortFwdWorker( unsigned int port );
    ~PortFwdWorker();

    unsigned int externalPort() const { return m_externalPort; }

    void unregister();
    
signals:
    void externalAddressDetected( QHostAddress ha, unsigned int port );

public slots:
    void work();

private:
    Portfwd* m_portfwd;
    QHostAddress m_externalAddress;
    unsigned int m_externalPort, m_port;
};


class PortFwdThread : public QThread
{
Q_OBJECT

public:
    explicit PortFwdThread( unsigned int port );
    ~PortFwdThread();

    QPointer< PortFwdWorker > worker() const;

signals:
    void externalAddressDetected( QHostAddress ha, unsigned int port );
    
protected:
    void run();

private:
    QPointer< PortFwdWorker > m_worker;
    unsigned int m_port;
};

#endif // PORTFWDTHREAD_H
