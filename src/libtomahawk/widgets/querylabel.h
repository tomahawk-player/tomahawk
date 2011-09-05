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

#include <QFrame>
#include <QTime>

#include "result.h"
#include "query.h"
#include "typedefs.h"
#include "dllmacro.h"

class DLLEXPORT QueryLabel : public QFrame
{
Q_OBJECT

public:
    enum DisplayType
    {
        None = 0,
        Artist = 1,
        Album = 2,
        Track = 4,
        ArtistAndAlbum = 3,
        ArtistAndTrack = 5,
        AlbumAndTrack = 6,
        Complete = 7
    };

    explicit QueryLabel( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    explicit QueryLabel( DisplayType type = Complete, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    explicit QueryLabel( const Tomahawk::result_ptr& result, DisplayType type = Complete, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    explicit QueryLabel( const Tomahawk::query_ptr& query, DisplayType type = Complete, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    virtual ~QueryLabel();

    QString text() const;
    QString artist() const;
    QString album() const;
    QString track() const;

    Tomahawk::result_ptr result() const { return m_result; }
    Tomahawk::query_ptr query() const { return m_query; }

    DisplayType type() const { return m_type; }
    void setType( DisplayType type ) { m_type = type; }

    Qt::Alignment alignment() const;
    void setAlignment( Qt::Alignment alignment );

    Qt::TextElideMode elideMode() const;
    void setElideMode( Qt::TextElideMode mode );

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void init();
    void updateLabel();

public slots:
    void setText( const QString& text );
    void setResult( const Tomahawk::result_ptr& result );
    void setQuery( const Tomahawk::query_ptr& query );

signals:
    void clicked();
    void clickedArtist();
    void clickedAlbum();
    void clickedTrack();

    void textChanged( const QString& text );
    void resultChanged( const Tomahawk::result_ptr& result );
    void queryChanged( const Tomahawk::query_ptr& query );

protected:
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void leaveEvent( QEvent* event );

    virtual void changeEvent( QEvent* event );
    virtual void paintEvent( QPaintEvent* event );

    virtual void startDrag();
    
private:
    QString smartAppend( QString& text, const QString& appendage ) const;
    QTime time;

    DisplayType m_type;
    QString m_text;
    Tomahawk::result_ptr m_result;
    Tomahawk::query_ptr m_query;

    Qt::Alignment align;
    Qt::TextElideMode mode;

    DisplayType m_hoverType;
    QRect m_hoverArea;
    QPoint m_dragPos;
    QMargins m_textMargins;
};

#endif // QUERYLABEL_H
