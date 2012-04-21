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

#ifndef CONTEXTWIDGET_H
#define CONTEXTWIDGET_H

#include <QGraphicsView>

#include "DllMacro.h"

#include "Query.h"

class QGraphicsScene;
class QGraphicsWebView;
class QGraphicsWidget;
class QTimeLine;

namespace Tomahawk
{
    class ContextPage;
    class ContextProxyPage;
}

namespace Ui
{
    class ContextWidget;
}

class DLLEXPORT ContextWidget : public QWidget
{
Q_OBJECT

public:
    ContextWidget( QWidget* parent = 0 );
    ~ContextWidget();

public slots:
    void setArtist( const Tomahawk::artist_ptr& artist );
    void setAlbum( const Tomahawk::album_ptr& album );
    void setQuery( const Tomahawk::query_ptr& query, bool force = false );

    void toggleSize();

private slots:
    void onPageFocused();

    void onAnimationStep( int frame );
    void onAnimationFinished();

protected:
    void paintEvent( QPaintEvent* e );
    void resizeEvent( QResizeEvent* e );
    void changeEvent( QEvent* e );

private:
    void fadeOut( bool animate );

    void layoutViews( bool animate = true );

    Ui::ContextWidget* ui;

    int m_minHeight;
    int m_maxHeight;
    QTimeLine* m_timeLine;

    QGraphicsScene* m_scene;
    QList<Tomahawk::ContextPage*> m_views;
    QList<Tomahawk::ContextProxyPage*> m_pages;

    int m_currentView;

    Tomahawk::artist_ptr m_artist;
    Tomahawk::album_ptr m_album;
    Tomahawk::query_ptr m_query;
    bool m_visible;
};

#endif // CONTEXTWIDGET_H
