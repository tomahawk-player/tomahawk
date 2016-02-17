/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TRACKDETAILVIEW_H
#define TRACKDETAILVIEW_H

#include <QWidget>

#include "Query.h"
#include "utils/DpiScaler.h"
#include "DownloadManager.h"
#include "DllMacro.h"

class QLabel;
class CaptionLabel;
class PlayableCover;
class QueryLabel;
class QScrollArea;
class QPushButton;

class DLLEXPORT TrackDetailView : public QWidget, private TomahawkUtils::DpiScaler
{
Q_OBJECT

public:
    explicit TrackDetailView( QWidget* parent = 0 );
    ~TrackDetailView();

    void setBuyButtonVisible( bool visible );

public slots:
    virtual void setQuery( const Tomahawk::query_ptr& query );
    void setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface );

signals:
    void downloadAll();
    void downloadCancel();

protected:

protected slots:

private slots:
    void onAlbumUpdated();
    void onCoverUpdated();
    void onSocialActionsLoaded();
    void onResultsChanged();

    void onBuyButtonClicked();
    void onDownloadManagerStateChanged( DownloadManager::DownloadManagerState newState, DownloadManager::DownloadManagerState oldState );

private:
    void setSocialActions();

    Tomahawk::playlistinterface_ptr m_playlistInterface;

    PlayableCover* m_playableCover;
    QueryLabel* m_nameLabel;
    QLabel* m_dateLabel;
    QLabel* m_lovedIcon;
    QLabel* m_lovedLabel;
    CaptionLabel* m_resultsBoxLabel;
    QPushButton* m_buyButton;
    bool m_buyButtonVisible;

    QWidget* m_infoBox;
    QWidget* m_resultsBox;
    QScrollArea* m_resultsScrollArea;

    Tomahawk::query_ptr m_query;
    QPixmap m_pixmap;
};

#endif // TRACKDETAILVIEW_H
