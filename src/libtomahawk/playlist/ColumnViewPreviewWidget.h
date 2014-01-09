/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013-2014, Teo Mrnjavac <teo@kde.org>
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

#ifndef COLUMNVIEWPREVIEWWIDGET_H
#define COLUMNVIEWPREVIEWWIDGET_H

#include <QWidget>

#include "Query.h"
#include "DllMacro.h"

class ColumnView;
class QueryLabel;
class PlayableCover;
class QLabel;
class ScrollingLabel;

class DLLEXPORT ColumnViewPreviewWidget : public QWidget
{
Q_OBJECT

public:
    explicit ColumnViewPreviewWidget( ColumnView* parent );
    ~ColumnViewPreviewWidget();

    QSize minimumSize() const;

public slots:
    void setQuery( const Tomahawk::query_ptr& query );

private slots:
    void onCoverUpdated();
    void onArtistClicked();

private:
    Tomahawk::query_ptr m_query;

    PlayableCover* m_cover;

    QLabel* m_ageLabel;
    QLabel* m_ageValue;

    QLabel* m_bitrateLabel;
    QLabel* m_bitrateValue;

    QLabel* m_composerLabel;
    QLabel* m_composerValue;

    QLabel* m_durationLabel;
    QLabel* m_durationValue;

    QLabel* m_yearLabel;
    QLabel* m_yearValue;

    ScrollingLabel* m_trackLabel;

    QueryLabel* m_artistLabel;
};

#endif // COLUMNVIEWPREVIEWWIDGET_H
