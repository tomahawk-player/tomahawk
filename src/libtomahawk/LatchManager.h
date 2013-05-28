/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#ifndef LATCHMANAGER_H
#define LATCHMANAGER_H

#include "source_ptr.h"
#include "playlistinterface_ptr.h"

#include <QObject>

#include "DllMacro.h"

class QState;
class QStateMachine;

namespace Tomahawk
{

class DLLEXPORT LatchManager : public QObject
{
    Q_OBJECT
public:
    explicit LatchManager( QObject* parent = 0 );
    virtual ~LatchManager();

    bool isLatched( const source_ptr& src );

public slots:
    void latchRequest( const Tomahawk::source_ptr& source );
    void unlatchRequest( const Tomahawk::source_ptr& source );
    void catchUpRequest();
    void latchModeChangeRequest( const Tomahawk::source_ptr& source, bool realtime );

private slots:
    void playlistChanged( Tomahawk::playlistinterface_ptr );
    
private:
    enum State {
        NotLatched =  0,
        Latching,
        Latched
    };

    State m_state;
    source_ptr m_latchedOnTo;
    source_ptr m_waitingForLatch;
    playlistinterface_ptr m_latchedInterface;
};

}

#endif // LATCHMANAGER_H
