/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>>
 *   Copyright 2012,      Leo Franchi   <lfranchi@kde.org>
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

#ifndef TOMAHAWKACTIONCOLLECTION_H
#define TOMAHAWKACTIONCOLLECTION_H

#include "DllMacro.h"

#include <QtGui/QAction>
#include <QtGui/QMenuBar>

class DLLEXPORT ActionCollection : public QObject
{
    Q_OBJECT

public:
    // Categories for custom-registered actions
    enum ActionDestination {
//         Tracks, TODO
        LocalPlaylists = 0
    };

    static ActionCollection* instance();

    ActionCollection( QObject *parent );
    ~ActionCollection();

    void initActions();

    /**
     * This method returns a main menu bar, suitable for Windows, Mac and X11.
     */
    QMenuBar *createMenuBar( QWidget *parent );

    /**
     * Returns a QMenu with all the entries that would normally be in the main menu,
     * arranged in a sensible way. The compressed menu makes absolutely no sense on Mac,
     * and fairly little sense on Unity and other X11 desktop configurations which pull
     * out the menu bar from the window.
     */
    QMenu *createCompactMenu( QWidget *parent );

    QAction* getAction( const QString& name );
    QList< QAction* > getAction( ActionDestination category );
    QObject* actionNotifier( QAction* );

    /**
     * Add an action for a specific category. The action will show up
     *  where the relevant category is displayed.
     *
     *  e.g. if you register a Playlist action, it will be shown when
     *       there is a context menu shown for a playlist.
     *
     * When the QAction* is shown, it will have a "payload" property that is set
     *  to the <specific type> that is being shown.
     *
     * Additionally you can pass a QObject* that will be notified before the given
     *  action is shown. The slot "aboutToShow( QAction*, <specific type> ) will be called,
     *
     *
     * <specific type> corresponds to the category: playlist_ptr for Playlists, etc.
     *
     * The Action Collection takes ownership of the action. It's time to let go.
     */
    void addAction( ActionDestination category, QAction* action, QObject* notify = 0 );

    /**
     * Remove an action from one or all specific categories
     */
    void removeAction( QAction* action );
    void removeAction( QAction* action, ActionDestination category );

public slots:
    void togglePrivateListeningMode();

signals:
    void privacyModeChanged();

private:
    static ActionCollection* s_instance;

    QHash< QString, QAction* > m_actionCollection;
    QHash< ActionDestination, QList< QAction* > > m_categoryActions;
    QHash< QAction*, QObject* > m_actionNotifiers;
};

#endif
