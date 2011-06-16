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

#ifndef TOPBAR_H
#define TOPBAR_H

#include <QWidget>
#include <QLabel>
#include <QList>

#include "viewmanager.h"
#include "sourcelist.h"
#include "dllmacro.h"

namespace Ui
{
    class TopBar;
}

class DLLEXPORT TopBar : public QWidget
{
Q_OBJECT

public:
    TopBar( QWidget* parent = 0 );
    ~TopBar();

signals:
    void filterTextChanged( const QString& newtext );

    void flatMode();
    void artistMode();
    void albumMode();

public slots:
    void setNumSources( unsigned int );
    void setNumTracks( unsigned int );
    void setNumArtists( unsigned int );
    void setNumShown( unsigned int );

    void setStatsVisible( bool b );
    void setModesVisible( bool b );
    void setFilterVisible( bool b );

    void addSource();
    void removeSource();

    void setFilter( const QString& filter );

private slots:
    void onModeChanged( Tomahawk::PlaylistInterface::ViewMode mode );
    void onFlatMode();
    void onArtistMode();
    void onAlbumMode();

protected:
    void changeEvent( QEvent* e );
    void resizeEvent( QResizeEvent* e );

private:
    void fadeOutDude( unsigned int i );
    void fadeInDude( unsigned int i );

    Ui::TopBar* ui;

    unsigned int m_sources, m_tracks, m_artists, m_shown;
    QList<QLabel*> m_dudes;

    Tomahawk::source_ptr m_onesource;
};

#endif // TOPBAR_H
