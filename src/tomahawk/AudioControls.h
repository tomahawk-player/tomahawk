/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef AUDIOCONTROLS_H
#define AUDIOCONTROLS_H

#include "Result.h"
#include "PlaylistInterface.h"
#include "Query.h"
#include "utils/DpiScaler.h"
#include "widgets/BackgroundWidget.h"

#include <QTimer>
#include <QTimeLine>

#include <memory>

class QDropEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class SocialWidget;

namespace Ui
{
    class AudioControls;
}

class AudioControls : public BackgroundWidget, private TomahawkUtils::DpiScaler
{
Q_OBJECT

public:
    AudioControls( QWidget* parent = nullptr );
    ~AudioControls();

signals:
    void playPressed();
    void pausePressed();

public slots:
    void onRepeatModeChanged( Tomahawk::PlaylistModes::RepeatMode mode );
    void onShuffleModeChanged( bool enabled );

protected:
    void changeEvent( QEvent* e );
    void dragEnterEvent ( QDragEnterEvent* );
    void dragMoveEvent ( QDragMoveEvent* );
    void dropEvent ( QDropEvent* );

private slots:
    void phononTickCheckTimeout();

    void onPlaybackStarted( const Tomahawk::result_ptr result );
    void onPlaybackLoading( const Tomahawk::result_ptr result );
    void onPlaybackPaused();
    void onPlaybackResumed();
    void onPlaybackSeeked( qint64 msec );
    void onPlaybackStopped();

    void onPlaybackTimer( qint64 msElapsed );
    void onTrackPosition( float position );
    void onVolumeChanged( int volume );
    void onMutedChanged( bool muted );
    void onControlStateChanged();

    void onRepeatClicked();
    void onShuffleClicked();

    void onSocialButtonClicked();
    void onLoveButtonClicked( bool );
    void onOwnerButtonClicked();

    void droppedTracks( QList<Tomahawk::query_ptr> );

    void onCoverUpdated();
    void onSocialActionsLoaded();

    void onInfoSystemPushTypesUpdated( Tomahawk::InfoSystem::InfoTypeSet supportedTypes );

protected:

private:
    void setCover();
    void setSocialActions();

    std::unique_ptr<Ui::AudioControls> ui;

    QPointer<SocialWidget> m_socialWidget;

    Tomahawk::result_ptr m_currentTrack;
    Tomahawk::PlaylistModes::RepeatMode m_repeatMode;
    bool m_shuffled;
    bool m_shouldShowShareAction;

    QTimer m_phononTickCheckTimer;
    QTimeLine m_sliderTimeLine;
    bool m_seeked;
    bool m_haveTiming;
    qint64 m_lastSliderCheck;
    qint64 m_lastTextSecondShown;

    QWidget* m_parent;
};

#endif // AUDIOCONTROLS_H
