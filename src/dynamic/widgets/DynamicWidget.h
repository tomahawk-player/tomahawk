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

#include "typedefs.h"
#include "dynamic/DynamicPlaylist.h"
#include "dynamic/DynamicControl.h"

class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QComboBox;
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
    
    void loadDynamicPlaylist( const dynplaylist_ptr& playlist );
    
    PlaylistInterface* playlistInterface() const;
    
public slots:
    void onRevisionLoaded( const Tomahawk::DynamicPlaylistRevision& rev );
    
private slots:
    void generateOrStart();
    void modeChanged(int);
    void tracksGenerated( const QList< Tomahawk::query_ptr>& queries );
    
    void controlsChanged();
    void controlChanged( const Tomahawk::dyncontrol_ptr& control );
    
private:
    void applyModeChange( int mode );
    
    dynplaylist_ptr m_playlist;
    QVBoxLayout* m_layout;
    
    QLabel* m_headerText;
    QHBoxLayout* m_headerLayout;
    QComboBox* m_modeCombo;
    QComboBox* m_generatorCombo;
    QLabel* m_logo;
    QPushButton* m_generateButton;
    
    DynamicControlList* m_controls;
    AnimatedSplitter* m_splitter;
    
    PlaylistView* m_view;
    PlaylistModel* m_model;
};

};

#endif
