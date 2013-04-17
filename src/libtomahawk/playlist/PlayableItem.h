/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef PLAYABLEITEM_H
#define PLAYABLEITEM_H

#include <QAbstractItemModel>
#include <QHash>
#include <QPersistentModelIndex>
#include <QPixmap>

#include "Typedefs.h"
#include "DllMacro.h"

class DLLEXPORT PlayableItem : public QObject
{
Q_OBJECT

public:
    ~PlayableItem();

    explicit PlayableItem( PlayableItem* parent = 0 );
    explicit PlayableItem( const Tomahawk::artist_ptr& artist, PlayableItem* parent = 0, int row = -1 );
    explicit PlayableItem( const Tomahawk::album_ptr& album, PlayableItem* parent = 0, int row = -1 );
    explicit PlayableItem( const Tomahawk::result_ptr& result, PlayableItem* parent = 0, int row = -1 );
    explicit PlayableItem( const Tomahawk::query_ptr& query, PlayableItem* parent = 0, int row = -1 );
    explicit PlayableItem( const Tomahawk::plentry_ptr& entry, PlayableItem* parent = 0, int row = -1 );

    const Tomahawk::artist_ptr& artist() const { return m_artist; }
    const Tomahawk::album_ptr& album() const { return m_album; }
    const Tomahawk::query_ptr& query() const { return m_query; }
    const Tomahawk::plentry_ptr& entry() const { return m_entry; }
    const Tomahawk::result_ptr& result() const;

    PlayableItem* parent() const { return m_parent; }

    bool isPlaying() const { return m_isPlaying; }
    void setIsPlaying( bool b ) { m_isPlaying = b; emit dataChanged(); }
    bool fetchingMore() const { return m_fetchingMore; }
    void setFetchingMore( bool b ) { m_fetchingMore = b; }
    void requestRepaint() { emit dataChanged(); }

    QString name() const;
    QString artistName() const;
    QString albumName() const;

    QList<PlayableItem*> children;

    QPersistentModelIndex index;

signals:
    void dataChanged();

private slots:
    void onResultsChanged();

private:
    void init( PlayableItem* parent, int row = -1 );

    Tomahawk::artist_ptr m_artist;
    Tomahawk::album_ptr m_album;
    Tomahawk::result_ptr m_result;
    Tomahawk::query_ptr m_query;
    Tomahawk::plentry_ptr m_entry;

    PlayableItem* m_parent;
    bool m_fetchingMore;
    bool m_isPlaying;
};

#endif // PLAYABLEITEM_H
