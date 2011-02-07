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

class QSpinBox;
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QComboBox;
class PlaylistInterface;
class PlaylistModel;
class PlaylistView;
class AnimatedSplitter;
class QLabel;
class ReadOrWriteWidget;

namespace Tomahawk
{

class DynamicModel;


class DynamicView;


class CollapsibleControls;


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
    
    virtual QSize sizeHint() const;
    virtual void resizeEvent( QResizeEvent* );
    
public slots:
    void onRevisionLoaded( const Tomahawk::DynamicPlaylistRevision& rev );
    
private slots:
    void generateOrStart();
    void tracksGenerated( const QList< Tomahawk::query_ptr>& queries );
    void generatorError( const QString& title, const QString& content );
    
    void controlsChanged();
    void controlChanged( const Tomahawk::dyncontrol_ptr& control );
    
    void layoutSteerer();
private:
    void applyModeChange( int mode );
    
    dynplaylist_ptr m_playlist;
    QVBoxLayout* m_layout;
    bool m_resolveOnNextLoad;
    int m_seqRevLaunched; // if we shoot off multiple createRevision calls, we don'y want to set one of the middle ones
    
    // used in OnDemand mode
    bool m_runningOnDemand;
    QWidget* m_steering;
        
    // layout and stuff
    QLabel* m_headerText;
    QHBoxLayout* m_headerLayout;
    ReadOrWriteWidget* m_generatorCombo;
    QLabel* m_logo;
    QPushButton* m_generateButton;
    QSpinBox* m_genNumber;
    
    CollapsibleControls* m_controls;
    
    DynamicView* m_view;
    DynamicModel* m_model;
};

};

#endif
