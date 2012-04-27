/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Nokia Corporation
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "Qnr_IoDeviceStream.h"

#include <QtNetwork/QNetworkReply>

using namespace Tomahawk;

QNR_IODeviceStream::QNR_IODeviceStream(QIODevice* ioDevice, QObject* parent)
    : Phonon::AbstractMediaStream( parent ),
      _ioDevice(ioDevice),
      _networkReply(0)
{
    _ioDevice->reset();
    if (!_ioDevice->isOpen()) {
        _ioDevice->open(QIODevice::ReadOnly);
    }

    Q_ASSERT(ioDevice->isOpen());
    Q_ASSERT(ioDevice->isReadable());
//     streamSize = ioDevice->size();
//     streamSeekable = !ioDevice->isSequential();
//
    // Allow handling of QNetworkReplies WRT its isFinished() function..
    _networkReply = qobject_cast<QNetworkReply *>(_ioDevice);
}


QNR_IODeviceStream::~QNR_IODeviceStream()
{
}

void QNR_IODeviceStream::reset()
{
    _ioDevice->reset();
    //resetDone();
}

void QNR_IODeviceStream::needData()
{
    quint32 size = 4096;
    const QByteArray data = _ioDevice->read(size);
// #ifdef __GNUC__
// #warning TODO 4.5 - make sure we do not break anything without this, it is preventing IODs from working when they did not yet emit readyRead()
// #endif
//    if (data.isEmpty() && !d->ioDevice->atEnd()) {
//        error(Phonon::NormalError, d->ioDevice->errorString());
//    }
    writeData(data);
    if (_ioDevice->atEnd()) {
        // If the IO device was identified as QNetworkReply also take its
        // isFinished() into account, when triggering EOD.
        if (!_networkReply || _networkReply->isFinished()) {
            endOfData();
        }
    }
}

void QNR_IODeviceStream::seekStream(qint64 offset)
{
    _ioDevice->seek(offset);
    //seekStreamDone();
}


// vim: sw=4 sts=4 et tw=100
