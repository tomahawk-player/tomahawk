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

#ifndef TOMAHAWK_TRAYICON_H
#define TOMAHAWK_TRAYICON_H

#include <QtGui/QSystemTrayIcon>
#include <QtCore/QTimer>
#include <QtGui/QMenu>

#include "result.h"

class TomahawkTrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TomahawkTrayIcon( QObject* parent );
    virtual bool event( QEvent* e );

    void setShowHideWindow( bool show = true );

public slots:
    void setResult( const Tomahawk::result_ptr& result );

private slots:
    void onAnimationTimer();
    void onActivated( QSystemTrayIcon::ActivationReason reason );
    void showWindow();

    void enablePlay();
    void enablePause();

private:
    void refreshToolTip();
    ~TomahawkTrayIcon();

    QTimer m_animationTimer;
    Tomahawk::result_ptr m_currentTrack;

    QList<QPixmap> m_animationPixmaps;
    int m_currentAnimationFrame;

    QMenu* m_contextMenu;

    QAction* m_showWindowAction;
};

#endif // TOMAHAWK_TRAYICON_H

