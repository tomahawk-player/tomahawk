/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef MUSIXMATCHPLUGIN_H
#define MUSIXMATCHPLUGIN_H

#include "infosystem/infosystem.h"
#include "infosystem/infosystemworker.h"

class QNetworkReply;

namespace Tomahawk
{

namespace InfoSystem
{

class MusixMatchPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    MusixMatchPlugin();
    virtual ~MusixMatchPlugin();

public slots:
    void trackSearchSlot();
    void trackLyricsSlot();

protected slots:
    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );

    virtual void pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, Tomahawk::InfoSystem::PushInfoPair pushInfoPair, Tomahawk::InfoSystem::PushInfoFlags pushFlags )
    {
        Q_UNUSED( caller )
        Q_UNUSED( type)
        Q_UNUSED( pushInfoPair )
        Q_UNUSED( pushFlags )
    }

virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
    {
        Q_UNUSED( criteria );
        Q_UNUSED( requestData );
    }
    
private:
    bool isValidTrackData( Tomahawk::InfoSystem::InfoRequestData requestData );
    
    QString m_apiKey;
};

}

}

#endif // MUSIXMATCHPLUGIN_H
