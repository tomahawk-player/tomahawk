/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef SHAREDTIMELINE_H
#define SHAREDTIMELINE_H

#include <QObject>
#include <QTimeLine>

#include "DllMacro.h"

namespace TomahawkUtils
{

class DLLEXPORT SharedTimeLine : public QObject
{
Q_OBJECT

public:
    SharedTimeLine();

    virtual ~SharedTimeLine() {}

    int currentFrame() { return m_timeline.currentFrame(); }

    void setUpdateInterval( int msec ) { if ( msec != m_timeline.updateInterval() ) m_timeline.setUpdateInterval( msec ); }

signals:
    void frameChanged( int );

protected slots:
    // Would be nice to ifdef this, but we would need CMake 2.8.11 for that as a requirement
// #if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    virtual void connectNotify( const QMetaMethod& signal );
    virtual void disconnectNotify( const QMetaMethod& signal );
// #else
    virtual void connectNotify( const char *signal );
    virtual void disconnectNotify( const char *signal );
// #endif

private:
    int m_refcount;
    QTimeLine m_timeline;
};

}

#endif
