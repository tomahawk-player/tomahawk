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

#ifndef DATABASERESOLVER_H
#define DATABASERESOLVER_H

#include "resolvers/Resolver.h"
#include "Result.h"
#include "Artist.h"
#include "Album.h"

#include "DllMacro.h"

class DLLEXPORT DatabaseResolver : public Tomahawk::Resolver
{
Q_OBJECT

public:
    explicit DatabaseResolver( int weight );

    virtual QString name() const;
    virtual unsigned int weight() const { return m_weight; }
    virtual unsigned int preference() const { return 100; }
    virtual unsigned int timeout() const { return 0; }

public slots:
    virtual void resolve( const Tomahawk::query_ptr& query );

private slots:
    void gotResults( const Tomahawk::QID qid, QList< Tomahawk::result_ptr> results );
    void gotAlbums( const Tomahawk::QID qid, QList< Tomahawk::album_ptr> albums );
    void gotArtists( const Tomahawk::QID qid, QList< Tomahawk::artist_ptr> artists );

private:
    int m_weight;
};

#endif // DATABASERESOLVER_H
