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

#ifndef WHATSHOTWIDGET_H
#define WHATSHOTWIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QListWidgetItem>
#include <QtGui/QStyledItemDelegate>

#include "playlistinterface.h"
#include "infosystem/infosystem.h"
#include "viewpage.h"

#include "utils/tomahawkutils.h"

#include "dllmacro.h"

class ChartsPlaylistInterface;
class QSortFilterProxyModel;
class QStandardItemModel;
class QStandardItem;
class TreeModel;
class PlaylistModel;
class OverlayWidget;
class TreeProxyModel;
class AlbumModel;

namespace Ui
{
    class WhatsHotWidget;
}

/**
 * \class
 * \brief The tomahawk page that shows music charts.
 */
class DLLEXPORT WhatsHotWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    WhatsHotWidget( QWidget* parent = 0 );
    ~WhatsHotWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return tr( "Charts" ); }
    virtual QString description() const { return QString(); }

    virtual bool showStatsBar() const { return false; }
    virtual bool showInfoBar() const { return false; }

    virtual bool jumpToCurrentTrack();
    virtual bool isBeingPlayed() const;

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

public slots:

private slots:
    void fetchData();
    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString target );
    void leftCrumbIndexChanged( QModelIndex );
private:
    void setLeftViewArtists( TreeModel* artistModel );
    void setLeftViewAlbums( AlbumModel* albumModel );
    void setLeftViewTracks( PlaylistModel* trackModel );


    QStandardItem* parseNode( QStandardItem* parentItem, const QString &label, const QVariant &data );
    Ui::WhatsHotWidget *ui;
    Tomahawk::playlistinterface_ptr m_playlistInterface;

    QStandardItemModel* m_crumbModelLeft;
    QSortFilterProxyModel* m_sortedProxy;

    // Cache our model data
    QHash< QString, AlbumModel* > m_albumModels;
    QHash< QString, TreeModel* > m_artistModels;
    QHash< QString, PlaylistModel* > m_trackModels;
    QString m_queueItemToShow;
    QSet< QString > m_queuedFetches;
    QTimer* m_timer;

    friend class ChartsPlaylistInterface;
};

#endif // WHATSHOTWIDGET_H
