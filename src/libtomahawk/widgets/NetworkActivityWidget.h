/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef NETWORKACTIVITYWIDGET_H
#define NETWORKACTIVITYWIDGET_H

#include "ViewPage.h"

class AnimatedSpinner;
class PlaylistModel;
class QModelIndex;
class QStandardItemModel;
class QSortFilterProxyModel;
namespace Ui
{
    class NetworkActivityWidget;
}

class NetworkActivityWidget : public QWidget, public Tomahawk::ViewPage
{
    Q_OBJECT
public:
    NetworkActivityWidget(QWidget* parent = 0);
    ~NetworkActivityWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return tr( "Network Activity" ); }
    virtual QString description() const { return QString(); }

    virtual bool showInfoBar() const { return false; }
    virtual bool isBeingPlayed() const;

    virtual bool jumpToCurrentTrack();

    void fetchData();
signals:

private slots:
    void weeklyCharts( const QList<Tomahawk::track_ptr>& );
    void monthlyCharts( const QList<Tomahawk::track_ptr>& );
    void yearlyCharts( const QList<Tomahawk::track_ptr>& );

    void leftCrumbIndexChanged( const QModelIndex& );

private:
    void actualFetchData();
    void checkDone();

    QSharedPointer<Ui::NetworkActivityWidget> ui;
    Tomahawk::playlistinterface_ptr m_playlistInterface;
    AnimatedSpinner* m_spinner;
    QStandardItemModel* m_crumbModelLeft;
    QSortFilterProxyModel* m_sortedProxy;

    QPointer<PlaylistModel> m_weeklyChartsModel;
    QPointer<PlaylistModel> m_monthlyChartsModel;
    QPointer<PlaylistModel> m_yearlyChartsModel;

};

#endif // NETWORKACTIVITYWIDGET_H
