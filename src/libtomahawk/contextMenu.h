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

#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QSignalMapper>
#include <QMenu>

#include "typedefs.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT ContextMenu : public QMenu
{
Q_OBJECT

public:
    enum MenuActions
    { ActionPlay = 1, ActionQueue = 2, ActionDelete = 4, ActionCopyLink = 8 };

    explicit ContextMenu( QWidget* parent = 0 );

    int supportedActions() const { return m_supportedActions; }
    void setSupportedActions( int actions ) { m_supportedActions = actions; }

    void setQuery( const Tomahawk::query_ptr& query );
    void setQueries( const QList<Tomahawk::query_ptr>& queries );

signals:
    void triggered( int action );

private slots:
    void onTriggered( int action );

    void copyLink();
    void addToQueue();

private:
    QSignalMapper* m_sigmap;
    int m_supportedActions;
    QList<Tomahawk::query_ptr> m_queries;
};

}; // ns

#endif
