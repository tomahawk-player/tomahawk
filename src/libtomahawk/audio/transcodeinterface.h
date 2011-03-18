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

#ifndef TRANSCODEINTERFACE_H
#define TRANSCODEINTERFACE_H

#include <QStringList>
#include <QByteArray>
#include <QObject>
#include <QMutex>

#include "dllmacro.h"

class DLLEXPORT TranscodeInterface : public QObject
{
    Q_OBJECT

    public:
        virtual ~TranscodeInterface() {}

        virtual const QStringList supportedTypes() const = 0;

        virtual int needData() = 0;
        virtual bool haveData() = 0;

        virtual unsigned int preferredDataSize() = 0;

        virtual QByteArray data() = 0;

//        virtual void setBufferCapacity( int bytes ) = 0;
//        virtual int bufferSize() = 0;

    public slots:
        virtual void clearBuffers() = 0;
        virtual void onSeek( int seconds ) = 0;
        virtual void processData( const QByteArray& data, bool finish ) = 0;
};

#endif
