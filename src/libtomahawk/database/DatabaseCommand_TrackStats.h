/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef DATABASECOMMAND_TRACKSTATS_H
#define DATABASECOMMAND_TRACKSTATS_H

#include <QVariantMap>

#include "DatabaseCommand.h"
#include "Typedefs.h"
#include "Track.h"
#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT DatabaseCommand_TrackStats : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_TrackStats( const trackdata_ptr& track, QObject* parent = 0 );
    explicit DatabaseCommand_TrackStats( const artist_ptr& artist, QObject* parent = 0 );

    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "trackstats"; }

signals:
    void trackStats( unsigned int totalPlays, unsigned int chartPosition );
    void done( const QList< PlaybackLog >& playbackData );

private:
    trackdata_ptr m_track;
    artist_ptr m_artist;
};

}

#endif // DATABASECOMMAND_TRACKSTATS_H
