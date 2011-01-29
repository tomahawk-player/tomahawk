/****************************************************************************************
 * Copyright (c) 2010-2011 Leo Franchi <lfranchi@kde.org>                               *
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

#ifndef COLLAPSIBLE_CONTROLS_H
#define COLLAPSIBLE_CONTROLS_H

#include "typedefs.h"

#include <QWidget>

class QHBoxLayout;
class QTimeLine;
class QToolButton;
class QLabel;
class QStackedLayout;
namespace Tomahawk
{

class DynamicControlWrapper;
class DynamicControlList;

class CollapsibleControls : public QWidget
{
    Q_OBJECT
public:
    CollapsibleControls( QWidget* parent );
    CollapsibleControls( const dynplaylist_ptr& playlist, bool isLocal, QWidget* parent = 0 );
    virtual ~CollapsibleControls();
    
    void setControls( const dynplaylist_ptr& playlist, bool isLocal );
    QList< DynamicControlWrapper* > controls() const;
    
    virtual QSize sizeHint() const;
    
signals:
    void controlsChanged();
    void controlChanged( const Tomahawk::dyncontrol_ptr& control );
    
private slots:
    void toggleCollapse();
    
    void onAnimationStep( int );
    void onAnimationFinished();
    
private:
    void init();
    
    dynplaylist_ptr m_dynplaylist;
    QStackedLayout* m_layout;
    DynamicControlList* m_controls;
    bool m_isLocal;
    
    QWidget* m_summaryWidget;
    QHBoxLayout* m_summaryLayout;
    QLabel* m_summary;
    QStackedLayout* m_expandL;
    QToolButton* m_summaryExpand;
    
    // animations!
    QTimeLine* m_timeline;
    int m_animHeight;
    bool m_collapseAnimation;
};

}
#endif
