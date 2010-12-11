/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DYNAMIC_WIDGET_H
#define DYNAMIC_WIDGET_H

#include <QWidget>
#include <tomahawk/typedefs.h>

class PlaylistInterface;
class PlaylistModel;
class PlaylistView;
class AnimatedSplitter;
class QLabel;

namespace Tomahawk
{

class DynamicControlList;

/**
 * This class contains the dynamic playlist config and the playlist view itself
 */
class DynamicWidget : public QWidget
{
Q_OBJECT 
public:
    explicit DynamicWidget( const dynplaylist_ptr& playlist, QWidget* parent = 0);
    virtual ~DynamicWidget();
    
    void setPlaylist( const dynplaylist_ptr& playlist );
    
    PlaylistInterface* playlistInterface() const;
    
private:
    QLabel* m_header;
    dynplaylist_ptr m_playlist;
    
    DynamicControlList* m_controls;
    AnimatedSplitter* m_splitter;
    
    PlaylistView* m_view;
    PlaylistModel* m_model;
};

};

#endif
