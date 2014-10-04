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

#ifndef QUERYLABEL_H
#define QUERYLABEL_H

#include <QLabel>
#include <QTime>

#include "Result.h"
#include "Query.h"
#include "Typedefs.h"
#include "DllMacro.h"

namespace Tomahawk
{
    class ContextMenu;
};

class DLLEXPORT QueryLabel : public QLabel
{
Q_OBJECT

public:
    enum DisplayType
    {
        None = 0,
        Artist = 1,
        Album = 2,
        Track = 4,
    };

    explicit QueryLabel( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    explicit QueryLabel( DisplayType type, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    explicit QueryLabel( const Tomahawk::result_ptr& result, DisplayType type = None, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    explicit QueryLabel( const Tomahawk::query_ptr& query, DisplayType type = None, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    virtual ~QueryLabel();

    QString text() const;

    Tomahawk::result_ptr result() const { return m_result; }
    Tomahawk::query_ptr query() const { return m_query; }
    Tomahawk::artist_ptr artist() const;
    Tomahawk::album_ptr album() const;

    DisplayType type() const { return m_type; }
    void setType( DisplayType type );

    Qt::TextElideMode elideMode() const;
    void setElideMode( Qt::TextElideMode mode );

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void init();

public slots:
    void clear();
    void setText( const QString& text );
    void setResult( const Tomahawk::result_ptr& result );
    void setQuery( const Tomahawk::query_ptr& query );
    void setArtist( const Tomahawk::artist_ptr& artist );
    void setAlbum( const Tomahawk::album_ptr& album );

signals:
    void clicked();

    void textChanged( const QString& text );
    void resultChanged( const Tomahawk::result_ptr& result );
    void queryChanged( const Tomahawk::query_ptr& query );

protected:
    virtual void contextMenuEvent( QContextMenuEvent* event );

    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void leaveEvent( QEvent* event );

    virtual void changeEvent( QEvent* event );
    virtual void paintEvent( QPaintEvent* event );

    virtual void startDrag();

private slots:
    void onResultChanged();

private:
    QTime m_time;

    DisplayType m_type;
    QString m_text;

    Tomahawk::result_ptr m_result;
    Tomahawk::query_ptr m_query;
    Tomahawk::artist_ptr m_artist;
    Tomahawk::album_ptr m_album;

    Tomahawk::ContextMenu* m_contextMenu;

    Qt::TextElideMode m_mode;
    QPoint m_dragPos;

    bool m_hovering;
};

#endif // QUERYLABEL_H
