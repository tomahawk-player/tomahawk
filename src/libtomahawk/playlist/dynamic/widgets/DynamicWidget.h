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

#ifndef DYNAMIC_WIDGET_H
#define DYNAMIC_WIDGET_H

#include <QWidget>

#include "typedefs.h"
#include "viewpage.h"

#include "dynamic/DynamicPlaylist.h"
#include "dynamic/DynamicControl.h"
#include "dynamic/DynamicModel.h"

class LoadingSpinner;
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

class DynamicView;

class CollapsibleControls;


/**
 * This class contains the dynamic playlist config and the playlist view itself
 */
class DynamicWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT 
public:
    explicit DynamicWidget( const dynplaylist_ptr& playlist, QWidget* parent = 0);
    virtual ~DynamicWidget();
    
    void loadDynamicPlaylist( const dynplaylist_ptr& playlist );
    
    virtual PlaylistInterface* playlistInterface() const;
    
    virtual QSize sizeHint() const;
    virtual void resizeEvent( QResizeEvent* );
    virtual void showEvent(QShowEvent* );
    
    static void paintRoundedFilledRect( QPainter& p, QPalette& pal, QRect& r, qreal opacity = .95 );

    virtual QWidget* widget() { return this; }
    
    virtual QString title() const { return m_model->title(); }
    virtual QString description() const { return m_model->description(); }
    
    virtual bool jumpToCurrentTrack();
    
public slots:
    void onRevisionLoaded( const Tomahawk::DynamicPlaylistRevision& rev );
    void playlistTypeChanged(QString);
    
    void startStation();
    void stopStation( bool stopPlaying = true );
    
    void trackStarted();
    void stationFailed( const QString& );
    
    void playlistStopped( PlaylistInterface* );
    void tracksAdded();
    
private slots:
    void generate( int = -1 );
    void tracksGenerated( const QList< Tomahawk::query_ptr>& queries );
    void generatorError( const QString& title, const QString& content );
    
    void controlsChanged();
    void controlChanged( const Tomahawk::dyncontrol_ptr& control );
    void showPreview();
    
    void layoutFloatingWidgets();

private:    
    dynplaylist_ptr m_playlist;
    QVBoxLayout* m_layout;
    bool m_resolveOnNextLoad;
    int m_seqRevLaunched; // if we shoot off multiple createRevision calls, we don'y want to set one of the middle ones

    // loading animation
    LoadingSpinner* m_loading;
    
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
