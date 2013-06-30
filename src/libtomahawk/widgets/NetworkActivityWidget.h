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

#include "WidgetsDllMacro.h"

class AnimatedSpinner;
class NetworkActivityWidgetPrivate;
class PlaylistModel;
class QModelIndex;
class QStandardItemModel;
class QSortFilterProxyModel;
namespace Ui
{
    class NetworkActivityWidget;
}

class TOMAHAWK_WIDGETS_EXPORT NetworkActivityWidget : public QWidget, public Tomahawk::ViewPage
{
    Q_OBJECT
public:
    enum ViewType { WeekChart = 1, MonthChart = 2, YearChart = 3, OverallChart = 4 };
    NetworkActivityWidget(QWidget* parent = 0);
    ~NetworkActivityWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const { return tr( "Network Activity" ); }
    virtual QString description() const { return QString(); }

    virtual bool showInfoBar() const { return false; }
    virtual bool isBeingPlayed() const;

    virtual bool jumpToCurrentTrack();
signals:

private slots:
    void weeklyCharts( const QList<Tomahawk::track_ptr>& );
    void monthlyCharts( const QList<Tomahawk::track_ptr>& );
    void yearlyCharts( const QList<Tomahawk::track_ptr>& );
    void overallCharts( const QList<Tomahawk::track_ptr>& );

    void leftCrumbIndexChanged( const QModelIndex& );

private:
    void fetchWeekCharts();
    void fetchMonthCharts();
    void fetchYearCharts();
    void fetchOverallCharts();

    void showWeekCharts();
    void showMonthCharts();
    void showYearCharts();
    void showOverallCharts();

    Q_DECLARE_PRIVATE( NetworkActivityWidget )
    NetworkActivityWidgetPrivate* d_ptr;
};

#endif // NETWORKACTIVITYWIDGET_H
