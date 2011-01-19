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

#ifndef DYNAMIC_CONTROL_LIST_H
#define DYNAMIC_CONTROL_LIST_H

#include "utils/animatedsplitter.h"
#include "typedefs.h"
#include "dynamic/DynamicPlaylist.h"

class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QToolButton;

namespace Tomahawk
{

class DynamicControlWidget;

  
/**
 * This widget encapsulates the list of dynamic controls and behaves as an animated widget in
 *  the animated splitter
 */

class DynamicControlList : public AnimatedWidget
{
    Q_OBJECT    
public:
    DynamicControlList(); // bad compiler!
    explicit DynamicControlList(AnimatedSplitter* parent );
    explicit DynamicControlList( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, AnimatedSplitter* parent, bool isLocal );
    virtual ~DynamicControlList();
    
    void setControls( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, bool isLocal );
    QList< DynamicControlWidget* > controls() const { return m_controls; }
    
    virtual void paintEvent(QPaintEvent* );
    
signals:
    void controlsChanged();
    void controlChanged( const Tomahawk::dyncontrol_ptr& control );
    
public slots:
    virtual void onHidden(QWidget* );
    virtual void onShown(QWidget* );
    void addNewControl();
    void removeControl();
    void controlChanged();
    
private:
    void init();
    
    geninterface_ptr m_generator;
    
    QVBoxLayout* m_layout;
    QList< DynamicControlWidget* > m_controls;
    QWidget* m_summaryWidget;
    
    QHBoxLayout* m_collapseLayout;
    QPushButton* m_collapse;
    QToolButton* m_addControl;
    
    bool m_isLocal;
};

};

#endif
