/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef STREAMCONNECTION_H
#define STREAMCONNECTION_H

#include <QObject>
#include <QSharedPointer>
#include <QIODevice>

#include "network/Connection.h"
#include "Result.h"

#include "DllMacro.h"

class ControlConnection;
class BufferIODevice;

class DLLEXPORT StreamConnection : public Connection
{
Q_OBJECT

public:
    enum Type
    {
        SENDING = 0,
        RECEIVING = 1
    };

    // RX:
    explicit StreamConnection( Servent* s, ControlConnection* cc, QString fid, const Tomahawk::result_ptr& result );
    // TX:
    explicit StreamConnection( Servent* s, ControlConnection* cc, QString fid );

    virtual ~StreamConnection();

    QString id() const;
    void setup();
    Connection* clone();

    const QSharedPointer<QIODevice>& iodevice() { return m_iodev; }
    ControlConnection* controlConnection() const { return m_cc; }

    Tomahawk::source_ptr source() const;
    Tomahawk::result_ptr track() const { return m_result; }
    qint64 transferRate() const { return m_transferRate; }

    Type type() const { return m_type; }
    QString fid() const { return m_fid; }

signals:
    void updated();

protected slots:
    virtual void handleMsg( msg_ptr msg );

private slots:
    void startSending( const Tomahawk::result_ptr& result );
    void reallyStartSending( const Tomahawk::result_ptr& result, QSharedPointer< QIODevice >& io ); //only called back from startSending
    void sendSome();
    void showStats( qint64 tx, qint64 rx );

    void onBlockRequest( int pos );

private:
    QSharedPointer<QIODevice> m_iodev;
    ControlConnection* m_cc;
    QString m_fid;
    Type m_type;
    QSharedPointer<QIODevice> m_readdev;

    int m_curBlock;

    int m_badded, m_bsent;
    bool m_allok; // got last msg ok, transfer complete?

    Tomahawk::source_ptr m_source;
    Tomahawk::result_ptr m_result;
    qint64 m_transferRate;
};

#endif // STREAMCONNECTION_H
