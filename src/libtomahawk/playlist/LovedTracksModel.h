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

#ifndef LOVEDTRACKSMODEL_H
#define LOVEDTRACKSMODEL_H

#include <QList>
#include <QHash>

#include "Typedefs.h"
#include "PlaylistModel.h"

#include "DllMacro.h"

class DLLEXPORT LovedTracksModel : public PlaylistModel
{
Q_OBJECT

public:
    explicit LovedTracksModel( QObject* parent = 0 );
    ~LovedTracksModel();

    unsigned int limit() const { return m_limit; }
    void setLimit( unsigned int limit ) { m_limit = limit; }

    bool isTemporary() const;

public slots:
    void setSource( const Tomahawk::source_ptr& source );

private slots:
    void onSourcesReady();
    void onSourceAdded( const Tomahawk::source_ptr& source );

    void onTrackLoved();
    void loadTracks();

private:
    Tomahawk::source_ptr m_source;
    unsigned int m_limit;
};

#endif // LOVEDTRACKSMODEL_H
