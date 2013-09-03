/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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
#ifndef TOMAHAWK_DATABASECOMMAND_TRENDINGARTISTS_H
#define TOMAHAWK_DATABASECOMMAND_TRENDINGARTISTS_H

#include "Artist.h"
#include "DatabaseCommand.h"

namespace Tomahawk {

class DatabaseCommand_TrendingArtistsPrivate;

class DLLEXPORT DatabaseCommand_TrendingArtists : public Tomahawk::DatabaseCommand
{
    Q_OBJECT
public:
    explicit DatabaseCommand_TrendingArtists( QObject* parent = 0 );
    virtual ~DatabaseCommand_TrendingArtists();

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "trendingartists"; }

    void setLimit( unsigned int amount );

signals:
    void done( const QList<QPair< double, Tomahawk::artist_ptr > >& artists );

private:
    Q_DECLARE_PRIVATE( DatabaseCommand_TrendingArtists )

};

} // namespace Tomahawk

#endif // TOMAHAWK_DATABASECOMMAND_TRENDINGARTISTS_H
