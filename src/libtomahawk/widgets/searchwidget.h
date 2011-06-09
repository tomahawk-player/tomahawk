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

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QWidget>
#include <QTimer>

#include "result.h"
#include "playlistinterface.h"
#include "viewpage.h"

#include "dllmacro.h"

class QPushButton;
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
    virtual PlaylistInterface* playlistInterface() const { return 0; }

    virtual QString title() const { return tr( "Search" ); }
    virtual QString description() const { return tr( "Results for '%1'" ).arg( m_search ); }

    virtual bool showStatsBar() const { return false; }

    virtual bool jumpToCurrentTrack() { return false; }

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

private slots:
    void onResultsFound( const QList<Tomahawk::result_ptr>& results );

    void cancel();

private:
    Ui::SearchWidget *ui;

    QString m_search;

    PlaylistModel* m_resultsModel;
    QList< Tomahawk::query_ptr > m_queries;
};

#endif // NEWPLAYLISTWIDGET_H
