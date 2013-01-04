/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012       Leo Franchi            <lfranchi@kde.org>
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

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QWidget>
#include <QTimer>

#include "Result.h"
#include "PlaylistInterface.h"
#include "ViewPage.h"

#include "DllMacro.h"

class QPushButton;
class PlayableModel;
class PlaylistModel;

namespace Ui
{
    class SearchWidget;
}

class DLLEXPORT SearchWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    SearchWidget( const QString& search, QWidget* parent = 0 );
    ~SearchWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return QString( tr( "Search: %1" ) ).arg( m_search ); }
    virtual QString description() const { return tr( "Results for '%1'" ).arg( m_search ); }
    virtual QPixmap pixmap() const;

    virtual bool isTemporaryPage() const { return true; }

    virtual bool jumpToCurrentTrack();

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

private slots:
    void onResultsFound( const QList<Tomahawk::result_ptr>& results );
    void onAlbumsFound( const QList<Tomahawk::album_ptr>& albums );
    void onArtistsFound( const QList<Tomahawk::artist_ptr>& artists );

    void onQueryFinished();

private:
    void updateArtists();
    void updateAlbums();

    Ui::SearchWidget *ui;

    QString m_search;

    PlayableModel* m_artistsModel;
    PlayableModel* m_albumsModel;
    PlaylistModel* m_resultsModel;

    QList< Tomahawk::query_ptr > m_queries;
    QMap< float, Tomahawk::artist_ptr > m_artists;
    QMap< float, Tomahawk::album_ptr > m_albums;
};

#endif // NEWPLAYLISTWIDGET_H
