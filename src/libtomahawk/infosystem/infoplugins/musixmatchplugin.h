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
    MusixMatchPlugin( InfoSystemWorker *parent );
    virtual ~MusixMatchPlugin();

public slots:
    void trackSearchSlot();
    void trackLyricsSlot();

protected slots:
    virtual void getInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input, const Tomahawk::InfoSystem::InfoCustomData customData );
    
private:
    bool isValidTrackData( const QString &caller, const QVariant &input, const InfoCustomData &customData );
    
    QString m_apiKey;

    InfoSystemWorker* m_infoSystemWorker;
};

}

}

#endif // MUSIXMATCHPLUGIN_H
