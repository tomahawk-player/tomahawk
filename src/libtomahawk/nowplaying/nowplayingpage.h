/*
    Creates a Now Playing page as a place holder for Music Visuals.
    Copyright (C) 2012  Dinesh <saidinesh5@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef NOWPLAYINGPAGE_H
#define NOWPLAYINGPAGE_H

#include <viewpage.h>
#include <QWidget>
#include "VSXuWidget.h"

class NowPlayingPage : public QWidget,public Tomahawk::ViewPage
{
    Q_OBJECT

public:
    NowPlayingPage( QWidget *parent = 0 );
    virtual bool jumpToCurrentTrack() { return false; }
    virtual QString description() const { return QString("VSXu Music Visualizer"); }
    virtual QString title() const{ return QString("Vovoid VSXu Ultra"); }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const { return Tomahawk::playlistinterface_ptr(); }
    virtual QWidget* widget(){ return this; }

private:
    VSXuWidget *m_VSXuWidget;

};

#endif // NOWPLAYINGPAGE_H
