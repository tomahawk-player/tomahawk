/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011       Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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
#ifndef PLAYABLEMODEL_P_H
#define PLAYABLEMODEL_P_H

#include "PlayableModel.h"

#include "PlayableItem.h"

#include <QPixmap>
#include <QStringList>

class PlayableModelPrivate
{
public:
    PlayableModelPrivate( PlayableModel* q, bool _loading )
        : q_ptr( q )
        , rootItem( new PlayableItem( 0 ) )
        , readOnly( true )
        , loading( _loading )
    {
    }

    PlayableModel* q_ptr;
    Q_DECLARE_PUBLIC( PlayableModel )

private:
    PlayableItem* rootItem;
    QPersistentModelIndex currentIndex;
    Tomahawk::QID currentUuid;

    bool readOnly;

    QString title;
    QString description;
    QPixmap icon;

    QStringList header;

    bool loading;
};

#endif // PLAYABLEMODEL_P_H
