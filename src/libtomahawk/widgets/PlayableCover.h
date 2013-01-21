/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011 - 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef PLAYABLEPIXMAP_H
#define PLAYABLEPIXMAP_H

#include <QLabel>
#include <QPaintEvent>
#include <QResizeEvent>

#include "Artist.h"
#include "DllMacro.h"

class ImageButton;

namespace Tomahawk
{
    class ContextMenu;
};

/**
 * \class PlayableCover
 * \brief QLabel which shows a play/pause button on hovering.
 */
class DLLEXPORT PlayableCover : public QLabel
{
Q_OBJECT

public:
    PlayableCover( QWidget* parent = 0 );
    virtual ~PlayableCover();

    bool showText() const { return m_showText; }
    void setShowText( bool b );

    QPixmap pixmap() const { return m_pixmap; }

public slots:
    virtual void setArtist( const Tomahawk::artist_ptr& artist );
    virtual void setAlbum( const Tomahawk::album_ptr& album );
    virtual void setQuery( const Tomahawk::query_ptr& query );

    void setPixmap( const QPixmap& pixmap );

protected:
    virtual void resizeEvent( QResizeEvent* event );
    virtual void paintEvent( QPaintEvent* event );

    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );

    virtual void contextMenuEvent( QContextMenuEvent* event );
    
    void leaveEvent( QEvent* event );
    void enterEvent( QEvent* event );

private slots:
    void onClicked();

private:
    QPixmap m_pixmap;

    ImageButton* m_button;
    Tomahawk::ContextMenu* m_contextMenu;

    Tomahawk::artist_ptr m_artist;
    Tomahawk::album_ptr m_album;
    Tomahawk::query_ptr m_query;

    QList< QRect > m_itemRects;
    QRect m_hoveredRect;

    bool m_showText;
};

#endif
