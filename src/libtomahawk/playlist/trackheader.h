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

#ifndef TRACKHEADER_H
#define TRACKHEADER_H

#include <QHeaderView>
#include <QSignalMapper>

#include "dllmacro.h"

class TrackView;

class DLLEXPORT TrackHeader : public QHeaderView
{
Q_OBJECT

public:
    explicit TrackHeader( TrackView* parent = 0 );
    ~TrackHeader();

    int visibleSectionCount() const;

public slots:
    void toggleVisibility( int index );
    void checkState();

protected:
    void contextMenuEvent( QContextMenuEvent* e );

private slots:
    void onSectionResized();
    void onToggleResizeColumns();

private:
    void addColumnToMenu( int index );

    TrackView* m_parent;

    QMenu* m_menu;
    QSignalMapper* m_sigmap;
    QList<QAction*> m_visActions;
    bool m_init;
};

#endif
