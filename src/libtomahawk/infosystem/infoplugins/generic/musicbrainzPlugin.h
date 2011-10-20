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

#ifndef MUSICBRAINZPLUGIN_H
#define MUSICBRAINZPLUGIN_H

#include "infosystem/infosystem.h"
#include "infosystem/infosystemworker.h"

class QNetworkReply;

namespace Tomahawk
{

namespace InfoSystem
{

class MusicBrainzPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    MusicBrainzPlugin();
    virtual ~MusicBrainzPlugin();

public slots:
    void namChangedSlot( QNetworkAccessManager *nam );

protected slots:
    virtual void getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );

    virtual void pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant data )
    {
        Q_UNUSED( caller );
        Q_UNUSED( type );
        Q_UNUSED( data );
    }

virtual void notInCacheSlot( uint requestId, Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
    {
        Q_UNUSED( requestId );
        Q_UNUSED( criteria );
        Q_UNUSED( requestData );
    }

private slots:
    void artistSearchSlot();
    void albumSearchSlot();
    void tracksSearchSlot();

    void albumFoundSlot();
    void tracksFoundSlot();

private:
    bool isValidTrackData( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );

    QWeakPointer< QNetworkAccessManager > m_nam;
};

}

}

#endif // MUSICBRAINZPLUGIN_H
