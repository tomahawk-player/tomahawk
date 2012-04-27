/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef CUSTOMPLAYLISTVIEW_H
#define CUSTOMPLAYLISTVIEW_H

#include "PlaylistView.h"

#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT CustomPlaylistView : public PlaylistView
{
    Q_OBJECT
public:
    enum PlaylistType {
        SourceLovedTracks,
        TopLovedTracks
    };

    explicit CustomPlaylistView( PlaylistType type, const source_ptr& s, QWidget* parent = 0 );
    virtual ~CustomPlaylistView();

    virtual bool showFilter() const { return false; }
    virtual bool showStatsBar() const { return false; }

    virtual QString title() const;
    virtual QPixmap pixmap() const;
    virtual QString description() const;
    virtual QString longDescription() const;

    virtual bool isTemporaryPage() const { return false; }
    virtual bool isBeingPlayed() const;
    virtual bool jumpToCurrentTrack();

private slots:
    void tracksGenerated( QList<Tomahawk::query_ptr> tracks );

    void socialAttributesChanged( const QString& );
    void sourceAdded( const Tomahawk::source_ptr& );

private:
    void generateTracks();

    PlaylistType m_type;
    source_ptr m_source;
    PlaylistModel* m_model;
};

}

#endif // CUSTOMPLAYLISTVIEW_H
