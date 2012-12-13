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

#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlist/PlayableProxyModel.h"
#include "playlist/PlaylistModel.h"
#include "TrackView.h"
#include "ViewPage.h"
#include "DllMacro.h"

class DLLEXPORT PlaylistView : public TrackView
{
Q_OBJECT

public:
    explicit PlaylistView( QWidget* parent = 0 );
    virtual ~PlaylistView();

    PlaylistModel* playlistModel() const { return m_model; }
    virtual void setPlaylistModel( PlaylistModel* model );
    virtual void setModel( QAbstractItemModel* model );

    virtual QList<Tomahawk::PlaylistUpdaterInterface*> updaters() const;

    virtual QPixmap pixmap() const;
    virtual bool isTemporaryPage() const;

signals:
    void nameChanged( const QString& title );
    void destroyed( QWidget* widget );
    void modelChanged();

protected:
    void keyPressEvent( QKeyEvent* event );
    bool eventFilter( QObject* obj, QEvent* event );

private slots:
    void onMenuTriggered( int action );

    void onDeleted();
    void onChanged();

private:
    PlaylistModel* m_model;
    QString m_customTitle;
    QString m_customDescripton;
};

#endif // PLAYLISTVIEW_H
