/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#pragma once
#ifndef LOVEDTRACKSMODEL_H
#define LOVEDTRACKSMODEL_H

#include "PlaylistModel.h"

class LovedTracksModelPrivate;

class DLLEXPORT LovedTracksModel : public PlaylistModel
{
    Q_OBJECT

public:
    virtual ~LovedTracksModel();

    unsigned int limit() const;
    void setLimit( unsigned int limit );

    bool isTemporary() const;

public slots:
    void setSource( const Tomahawk::source_ptr& source );

protected slots:
    virtual void loadTracks();

protected:
    explicit LovedTracksModel( QObject* parent = 0 );
    explicit LovedTracksModel( QObject* parent, LovedTracksModelPrivate* d );

private slots:
    void onSourcesReady();
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onTrackLoved();

    void tracksLoaded( QList<Tomahawk::query_ptr> );

private:
    Q_DECLARE_PRIVATE( LovedTracksModel )

    void init();

};

#endif // LOVEDTRACKSMODEL_H
