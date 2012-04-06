/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011-2012, Leo Franchi <lfranchi@kde.org>
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

#ifndef PIXMAPDELEGATEFADER_H
#define PIXMAPDELEGATEFADER_H

#include "artist.h"
#include "album.h"

#include <QObject>
#include <QTimeLine>
#include <QQueue>

namespace Tomahawk
{

/**
 * No parent, manage it yourself!
 *
 * TODO: Handle changing sizes
 */

class PixmapDelegateFader : public QObject
{
    Q_OBJECT
public:
    PixmapDelegateFader( const artist_ptr& artist, const QSize& size );
    PixmapDelegateFader( const album_ptr& album, const QSize& size );

    virtual ~PixmapDelegateFader();

    void setPixmap( const QPixmap& pixmap );

    QPixmap currentPixmap() const;

signals:
    void repaintRequest();

private slots:
    void artistChanged();
    void albumChanged();

    void onAnimationStep( int );
    void onAnimationFinished();
private:
    void init();

    artist_ptr m_artist;
    album_ptr m_album;
    QSize m_size;

    QQueue<QPixmap> m_pixmapQueue;
    QTimeLine m_crossfadeTimeline;
    QPixmap m_currentReference, m_oldReference, m_current;
};

}

#endif // PIXMAPDELEGATEFADER_H
