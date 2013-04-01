/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Casey Link <unnamedrambler@gmail.com>
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef NEWRELEASESWIDGET_H
#define NEWRELEASESWIDGET_H

#include "PlaylistInterface.h"
#include "infosystem/InfoSystem.h"
#include "ViewPage.h"

#include "DllMacro.h"

#include <QWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>

class QSortFilterProxyModel;
class QStandardItemModel;
class QStandardItem;
class TreeModel;
class PlaylistModel;
class TreeProxyModel;
class AlbumModel;
class PlayableModel;
class AnimatedSpinner;
namespace Ui
{
    class NewReleasesWidget;
}

namespace Tomahawk
{
    class ChartDataLoader;
    class ChartsPlaylistInterface;
    class ChartDataLoader;
}

/**
 * \class
 * \brief The tomahawk page that shows music charts.
 */
class DLLEXPORT NewReleasesWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    NewReleasesWidget( QWidget* parent = 0 );
    ~NewReleasesWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return tr( "New Releases" ); }
    virtual QString description() const { return QString(); }

    virtual bool showInfoBar() const { return false; }
    virtual bool isBeingPlayed() const;

    virtual bool jumpToCurrentTrack();

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

public slots:
    void fetchData();

private slots:
    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString target );
    void leftCrumbIndexChanged( QModelIndex );

    void newReleasesLoaded( Tomahawk::ChartDataLoader*, const QList< Tomahawk::album_ptr >& );

private:
    void setLeftViewArtists( TreeModel* artistModel );
    void setLeftViewAlbums( PlayableModel* albumModel );
    void setLeftViewTracks( PlaylistModel* trackModel );


    QStandardItem* parseNode( QStandardItem* parentItem, const QString &label, const QVariant &data );
    Ui::NewReleasesWidget *ui;
    Tomahawk::playlistinterface_ptr m_playlistInterface;

    QStandardItemModel* m_crumbModelLeft;
    QSortFilterProxyModel* m_sortedProxy;

    // Load artist, album, and track objects in a thread
    // {Artist,Album,Track}::get() calls are all synchronous db calls
    // and we don't want to lock up out UI in case the db is busy (e.g. on startup)
    QThread* m_workerThread;
    QSet< Tomahawk::ChartDataLoader* > m_workers;

    // Cache our model data
    QHash< QString, PlayableModel* > m_albumModels;
    QString m_queueItemToShow;
    QSet< QString > m_queuedFetches;
    AnimatedSpinner* m_spinner;
    bool m_loading;
    friend class Tomahawk::ChartsPlaylistInterface;
};

#endif // NEWRELEASESWIDGET_H
