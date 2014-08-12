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
#include "DllMacro.h"

class QLabel;
class QueryLabel;
class PlayableCover;

class DLLEXPORT TrackDetailView : public QWidget
{
Q_OBJECT

public:
    explicit TrackDetailView( QWidget* parent = 0 );
    ~TrackDetailView();

public slots:
    virtual void setQuery( const Tomahawk::query_ptr& query );

signals:

protected:

protected slots:

private slots:
    void onCoverUpdated();
    void onSocialActionsLoaded();
    void onResultsChanged();

private:
    void setSocialActions();

    PlayableCover* m_playableCover;
    QueryLabel* m_nameLabel;
    QLabel* m_dateLabel;
    QLabel* m_lovedIcon;
    QLabel* m_lovedLabel;

    QWidget* m_infoBox;
    QWidget* m_resultsBox;

    Tomahawk::query_ptr m_query;
    QPixmap m_pixmap;
};

#endif // TRACKDETAILVIEW_H
