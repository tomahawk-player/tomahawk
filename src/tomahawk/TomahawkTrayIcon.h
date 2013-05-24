/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include <QSystemTrayIcon>
#include <QTimer>
#include <QMenu>

#include "Result.h"

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

    void onPause();
    void onPlay();
    void onStop();
    void onResume();

    void onSocialActionsLoaded();
    void onStopContinueAfterTrackChanged();
    void stopContinueAfterTrackActionTriggered();
    void loveTrackTriggered();

    void menuAboutToShow();

private:
    void refreshToolTip();
    ~TomahawkTrayIcon();

    QTimer m_animationTimer;
    Tomahawk::result_ptr m_currentTrack;

    QList<QPixmap> m_animationPixmaps;
    int m_currentAnimationFrame;

    QMenu* m_contextMenu;

    QAction* m_showWindowAction;
    QAction* m_stopContinueAfterTrackAction;
    QAction* m_loveTrackAction;
};

#endif // TOMAHAWK_TRAYICON_H

