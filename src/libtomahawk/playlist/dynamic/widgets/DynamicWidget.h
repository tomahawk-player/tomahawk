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

class QShowEvent;
class QHideEvent;
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

class DynamicSetupWidget;


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
    virtual void hideEvent(QHideEvent* );
    virtual void showEvent(QShowEvent* );
    
    static void paintRoundedFilledRect( QPainter& p, QPalette& pal, QRect& r, qreal opacity = .95 );
public slots:
    void onRevisionLoaded( const Tomahawk::DynamicPlaylistRevision& rev );
    void playlistTypeChanged(QString);
    
    void startStation();
    void stopStation( bool stopPlaying = true );
    
    void playPressed();
    void pausePressed();
    void stationFailed( const QString& );
    
private slots:
    void generate( int = -1 );
    void tracksGenerated( const QList< Tomahawk::query_ptr>& queries );
    void generatorError( const QString& title, const QString& content );
    
    void controlsChanged();
    void controlChanged( const Tomahawk::dyncontrol_ptr& control );
    
    void layoutFloatingWidgets();
private:    
    dynplaylist_ptr m_playlist;
    QVBoxLayout* m_layout;
    bool m_resolveOnNextLoad;
    int m_seqRevLaunched; // if we shoot off multiple createRevision calls, we don'y want to set one of the middle ones
    
    // setup controls
    DynamicSetupWidget* m_setup;
    
    // used in OnDemand mode
    bool m_runningOnDemand;
    bool m_controlsChanged;
    QWidget* m_steering;
        
    CollapsibleControls* m_controls;
    
    DynamicView* m_view;
    DynamicModel* m_model;
};

};

#endif
