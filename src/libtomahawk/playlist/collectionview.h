/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

#include "trackproxymodel.h"
#include "trackview.h"
#include "ViewPage.h"

#include "DllMacro.h"

class TrackModel;

class DLLEXPORT CollectionView : public TrackView, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    explicit CollectionView( QWidget* parent = 0 );
    ~CollectionView();

    virtual void setTrackModel( TrackModel* model );
    virtual void setModel( QAbstractItemModel* model );

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const { return proxyModel()->playlistInterface(); }

    virtual QString title() const { return model()->title(); }
    virtual QString description() const { return model()->description(); }
    virtual QPixmap pixmap() const { return QPixmap( RESPATH "images/music-icon.png" ); }

    virtual bool showModes() const { return true; }
    virtual bool showFilter() const { return true; }

    virtual bool jumpToCurrentTrack();

private slots:
    void onTrackCountChanged( unsigned int tracks );

protected:
    virtual void dragEnterEvent( QDragEnterEvent* event );
};

#endif // COLLECTIONVIEW_H
