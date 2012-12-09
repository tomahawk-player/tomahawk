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

/**
 * \class ArtistInfoWidget
 * \brief ViewPage, which displays top-hits, related artists and albums for an artist.
 *
 * This Tomahawk ViewPage displays top-hits, related artists and known albums
 * for any given artist. It is our default ViewPage when showing an artist
 * via ViewManager.
 *
 */

#ifndef ARTISTINFOWIDGET_H
#define ARTISTINFOWIDGET_H

#include <QWidget>

#include "Typedefs.h"
#include "PlaylistInterface.h"
#include "ViewPage.h"

#include "DllMacro.h"

class PlayableModel;
class PlaylistModel;

namespace Ui
{
    class ArtistInfoWidget;
}

class MetaArtistInfoInterface;

class DLLEXPORT ArtistInfoWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    ArtistInfoWidget( const Tomahawk::artist_ptr& artist, QWidget* parent = 0 );
    ~ArtistInfoWidget();

    /** \brief Loads information for a given artist.
     *  \param artist The artist that you want to load information for.
     *
     *  Calling this method will make ArtistInfoWidget load information about
     *  an artist's top hits, related artists and all available albums. It is
     *  automatically called by the constructor, but you can use it to load
     *  another artist's information at any point.
     */
    void load( const Tomahawk::artist_ptr& artist );

    Tomahawk::artist_ptr artist() const { return m_artist; }

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return m_title; }
    virtual QString description() const { return m_description; }
    virtual QString longDescription() const { return m_longDescription; }
    virtual QPixmap pixmap() const { if ( m_pixmap.isNull() ) return Tomahawk::ViewPage::pixmap(); else return m_pixmap; }

    virtual bool isTemporaryPage() const { return true; }
    virtual bool showInfoBar() const { return false; }

    virtual bool jumpToCurrentTrack();
    virtual bool isBeingPlayed() const;

signals:
    void longDescriptionChanged( const QString& description );
    void descriptionChanged( const QString& description );
    void pixmapChanged( const QPixmap& pixmap );

protected:
    void changeEvent( QEvent* e );

private slots:
    void onArtistImageUpdated();
    void onBiographyLoaded();

    void onAlbumsFound( const QList<Tomahawk::album_ptr>& albums, Tomahawk::ModelMode mode );
    void onTracksFound( const QList<Tomahawk::query_ptr>& queries, Tomahawk::ModelMode mode );
    void onSimilarArtistsLoaded();

    void onBiographyLinkClicked( const QUrl& url );

private:
    Ui::ArtistInfoWidget *ui;

    Tomahawk::artist_ptr m_artist;

    PlayableModel* m_relatedModel;
    PlayableModel* m_albumsModel;
    PlaylistModel* m_topHitsModel;
    Tomahawk::playlistinterface_ptr m_plInterface;

    QString m_title;
    QString m_description;
    QString m_longDescription;
    QPixmap m_pixmap;

    friend class ::MetaArtistInfoInterface;
};

#endif // ARTISTINFOWIDGET_H
