/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef SOURCEINFOWIDGET_H
#define SOURCEINFOWIDGET_H

#include <QWidget>

#include "PlaylistInterface.h"
#include "ViewPage.h"

#include "DllMacro.h"

class AlbumModel;
class CollectionFlatModel;
class RecentlyAddedModel;
class RecentlyPlayedModel;

namespace Ui
{
    class SourceInfoWidget;
}

class DLLEXPORT SourceInfoWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    SourceInfoWidget( const Tomahawk::source_ptr& source, QWidget* parent = 0 );
    ~SourceInfoWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const { return Tomahawk::playlistinterface_ptr(); }

    virtual QString title() const { return m_title; }
    virtual QString description() const { return m_description; }
    virtual QPixmap pixmap() const;

    virtual bool jumpToCurrentTrack() { return false; }

protected:
    void changeEvent( QEvent* e );

private slots:
    void loadRecentAdditions();

    void onCollectionChanged();

private:
    Ui::SourceInfoWidget *ui;

    RecentlyAddedModel* m_recentTracksModel;
    RecentlyPlayedModel* m_historyModel;
    AlbumModel* m_recentAlbumModel;

    Tomahawk::source_ptr m_source;

    QString m_title;
    QString m_description;
};

#endif // SOURCEINFOWIDGET_H
