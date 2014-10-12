/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2014, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef TOMAHAWK_CHARTSWIDGET_H
#define TOMAHAWK_CHARTSWIDGET_H

#include "../ViewPageDllMacro.h"
#include "ViewPagePlugin.h"
#include "ViewPageLazyLoader.h"

class AnimatedSpinner;
class PlayableModel;
class QSortFilterProxyModel;
class QStandardItemModel;
class QStandardItem;
class TreeModel;

namespace Ui
{
    class ChartsWidget;
}

namespace Tomahawk
{
    class ChartDataLoader;
    class ChartsPlaylistInterface;
}

namespace Tomahawk
{
namespace Widgets
{


/**
 * \class
 * \brief The tomahawk page that shows music charts.
 */
class ChartsWidget : public QWidget
{
Q_OBJECT

public:
    ChartsWidget( QWidget* parent = 0 );
    ~ChartsWidget();

    Tomahawk::playlistinterface_ptr playlistInterface() const;
    bool isBeingPlayed() const;
    bool jumpToCurrentTrack();

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

    void chartArtistsLoaded( Tomahawk::ChartDataLoader*, const QList< Tomahawk::artist_ptr >& );
    void chartAlbumsLoaded( Tomahawk::ChartDataLoader*, const QList< Tomahawk::album_ptr >& );
    void chartTracksLoaded( Tomahawk::ChartDataLoader*, const QList< Tomahawk::query_ptr >& );

private:
    void setLeftViewArtists( PlayableModel* artistModel );
    void setLeftViewAlbums( PlayableModel* albumModel );
    void setLeftViewTracks( PlayableModel* trackModel );
    void setViewData( const QVariantMap& data );

    QStandardItem* parseNode( QStandardItem* parentItem, const QString& label, const QVariant& data );

    Ui::ChartsWidget* ui;
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
    QHash< QString, PlayableModel* > m_artistModels;
    QHash< QString, PlayableModel* > m_trackModels;
    QString m_queueItemToShow;
    QSet< QString > m_queuedFetches;
    QMap<QString, QVariant> m_currentVIds;

    AnimatedSpinner* m_spinner;
    bool m_loading;
    friend class Tomahawk::ChartsPlaylistInterface;
};

const QString CHARTS_VIEWPAGE_NAME = "charts";

class TOMAHAWK_VIEWPAGE_EXPORT ChartsPage : public Tomahawk::ViewPageLazyLoader< ChartsWidget >
{
Q_OBJECT
Q_INTERFACES( Tomahawk::ViewPagePlugin )
Q_PLUGIN_METADATA( IID "org.tomahawk-player.Player.ViewPagePlugin" )

public:
    ChartsPage( QWidget* parent = 0 );
    virtual ~ChartsPage();

    const QString defaultName() Q_DECL_OVERRIDE { return CHARTS_VIEWPAGE_NAME; }
    QString title() const Q_DECL_OVERRIDE { return tr( "Charts" ); }
    QString description() const Q_DECL_OVERRIDE { return QString(); }

    const QString pixmapPath() const Q_DECL_OVERRIDE { return ( RESPATH "images/charts.svg" ); }

    int sortValue() Q_DECL_OVERRIDE { return 5; }
};


} // Widgets
} // Tomahawk

#endif // TOMAHAWK_CHARTSWIDGET_H
