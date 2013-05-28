/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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


#ifndef INBOXVIEW_H
#define INBOXVIEW_H

#include "TrackView.h"

class InboxView : public TrackView
{
    Q_OBJECT
public:
    explicit InboxView( QWidget* parent = 0 );

public slots:
    /**
     * Reimplemented in order to ignore PlayableModel::isReadOnly()
     */
    virtual void deleteSelectedItems();

    virtual void onMenuTriggered( int action );
};

#endif // INBOXVIEW_H
