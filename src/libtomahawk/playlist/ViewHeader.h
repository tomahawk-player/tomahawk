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

#ifndef VIEWHEADER_H
#define VIEWHEADER_H

#include <QHeaderView>
#include <QSignalMapper>

#include "DllMacro.h"

class DLLEXPORT ViewHeader : public QHeaderView
{
Q_OBJECT

public:
    explicit ViewHeader( QAbstractItemView* parent = 0 );
    ~ViewHeader();

    int visibleSectionCount() const;

    void setDefaultColumnWeights( QList<double> weights ) { m_columnWeights = weights; }

    QString guid() const { return m_guid; }
    void setGuid( const QString& guid );

public slots:
    void toggleVisibility( int index );
    bool checkState();

protected:
    void contextMenuEvent( QContextMenuEvent* e );

private slots:
    virtual void onSectionsChanged();
    void onToggleResizeColumns();

private:
    void addColumnToMenu( int index );

    QAbstractItemView* m_parent;
    QString m_guid;
    QList<double> m_columnWeights;
    QMenu* m_menu;
    QSignalMapper* m_sigmap;
    QList<QAction*> m_visActions;
    bool m_init;
};

#endif
