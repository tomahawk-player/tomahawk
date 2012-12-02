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

#ifndef QMLGRIDVIEW_H
#define QMLGRIDVIEW_H

#include <QDeclarativeView>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "ViewPage.h"
#include "PlayableProxyModel.h"
#include "widgets/OverlayWidget.h"
#include "DllMacro.h"

namespace Tomahawk
{
    class ContextMenu;
};

class AnimatedSpinner;
class GridItemDelegate;
class PlayableModel;
class GridPlaylistInterface;

class DLLEXPORT QmlGridView : public QDeclarativeView, public Tomahawk::ViewPage
{
Q_OBJECT
public:
    QmlGridView( QWidget *parent = 0);
    ~QmlGridView();

    void setPlayableModel( PlayableModel* model );
//    void setModel( QAbstractItemModel* model );
    PlayableProxyModel* proxyModel() const { return m_proxyModel; }


    QWidget *widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const { return m_playlistInterface; }
    virtual QString title() const { return m_model->title(); }
    virtual QString description() const { return m_model->description(); }
    virtual bool jumpToCurrentTrack() { return false; }

    Q_INVOKABLE void onItemClicked( int index );
    Q_INVOKABLE void onItemPlayClicked( int index );

private:
    PlayableModel* m_model;
    PlayableProxyModel* m_proxyModel;
    Tomahawk::playlistinterface_ptr m_playlistInterface;

signals:
    void modelChanged();

};

#endif
