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
#ifndef TOPLOVEDTRACKSMODEL_H
#define TOPLOVEDTRACKSMODEL_H

#include "LovedTracksModel.h"

class TopLovedTracksModelPrivate;

class DLLEXPORT TopLovedTracksModel : public LovedTracksModel
{
    Q_OBJECT

  public:
    explicit TopLovedTracksModel( QObject* parent = 0 );
    virtual ~TopLovedTracksModel();

  protected slots:
    void loadTracks();

  private:
    Q_DECLARE_PRIVATE( TopLovedTracksModel )
};

#endif // TOPLOVEDTRACKSMODEL_H
