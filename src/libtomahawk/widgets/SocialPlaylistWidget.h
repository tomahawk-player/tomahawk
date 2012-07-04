/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

/**
 * \class SocialPlaylistWidget
 * \brief ViewPage, which displays some interesting lists of songs mined from the database
 *
 * This Tomahawk ViewPage displays various interesting database searches that expose cool
 *  lists of songs. It is accessed from the sidebar, and contains a few custom-created DatabaseGenerator-backed
 *  playlists.
 *
 */

#ifndef SOCIALPLAYLISTWIDGET_H
#define SOCIALPLAYLISTWIDGET_H

#include <QWidget>

#include "ViewPage.h"
#include "DllMacro.h"
#include "Typedefs.h"
#include "Album.h"
#include "Query.h"

class PlayableModel;
class PlaylistModel;
class TreeModel;

class Ui_SocialPlaylistWidget;

namespace Tomahawk
{

class DLLEXPORT SocialPlaylistWidget : public QWidget, public Tomahawk::ViewPage
{
    Q_OBJECT

public:
    SocialPlaylistWidget( QWidget* parent = 0 );
    ~SocialPlaylistWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return m_title; }
    virtual QString description() const { return m_description; }
    virtual QString longDescription() const { return m_longDescription; }
    virtual QPixmap pixmap() const { if ( m_pixmap.isNull() ) return Tomahawk::ViewPage::pixmap(); else return m_pixmap; }
    virtual bool jumpToCurrentTrack() { return false; }

signals:
    void longDescriptionChanged( const QString& description );
    void descriptionChanged( const QString& description );
    void pixmapChanged( const QPixmap& pixmap );

private slots:
    void popularAlbumsFetched( QList<Tomahawk::album_ptr> );
    void topForeignTracksFetched( QList<Tomahawk::query_ptr> );

private:
    void fetchFromDB();

    Ui_SocialPlaylistWidget *ui;
    PlaylistModel* m_topForeignTracksModel;
    PlayableModel* m_popularNewAlbumsModel;

    QString m_title;
    QString m_description;
    QString m_longDescription;
    QPixmap m_pixmap;

    static QString s_popularAlbumsQuery;
    static QString s_mostPlayedPlaylistsQuery;
    static QString s_topForeignTracksQuery;
};

}

#endif // SOCIALPLAYLISTWIDGET_H
