/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

/**
 * \class TrackInfoWidget
 * \brief ViewPage, which displays a track
 *
 * This Tomahawk ViewPage displays a track
 * It is our default ViewPage when showing an track via ViewManager.
 *
 */

#ifndef TRACKINFOWIDGET_H
#define TRACKINFOWIDGET_H

#include <QtGui/QWidget>

#include "PlaylistInterface.h"
#include "ViewPage.h"
#include "infosystem/InfoSystem.h"

#include "DllMacro.h"
#include "Typedefs.h"

class PlayableModel;

namespace Ui
{
    class TrackInfoWidget;
}

class DLLEXPORT TrackInfoWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    TrackInfoWidget( const Tomahawk::query_ptr& query, QWidget* parent = 0 );
    ~TrackInfoWidget();

    Tomahawk::query_ptr query() const { return m_query; }

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return m_title; }
    virtual QString description() const { return QString(); }
    virtual QString longDescription() const { return QString(); }
    virtual QPixmap pixmap() const { if ( m_pixmap.isNull() ) return Tomahawk::ViewPage::pixmap(); else return m_pixmap; }

    virtual bool isBeingPlayed() const;
    virtual bool isTemporaryPage() const { return true; }
    virtual bool showInfoBar() const { return false; }

    virtual bool jumpToCurrentTrack() { return false; }

public slots:
    void load( const Tomahawk::query_ptr& query );

protected:
    void changeEvent( QEvent* e );

private slots:
    void onCoverUpdated();
    void onStatsLoaded();
    void onSimilarArtistsLoaded();
    void onSimilarTracksLoaded();
    void onLyricsLoaded();

    void onArtistClicked();
    void onAlbumClicked();

private:
    Ui::TrackInfoWidget *ui;

    Tomahawk::query_ptr m_query;
    Tomahawk::artist_ptr m_artist;

    PlayableModel* m_relatedTracksModel;
    QString m_title;
    QPixmap m_pixmap;
};

#endif // TRACKINFOWIDGET_H
