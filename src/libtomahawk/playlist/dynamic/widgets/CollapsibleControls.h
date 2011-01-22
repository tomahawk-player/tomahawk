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
    CollapsibleControls( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, bool isLocal, QWidget* parent = 0 );
    virtual ~CollapsibleControls();
    
    void setControls( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, bool isLocal );
    QList< DynamicControlWrapper* > controls() const;
    
signals:
    void controlsChanged();
    void controlChanged( const Tomahawk::dyncontrol_ptr& control );
    
private slots:
    void toggleCollapse();
    
private:
    void init();
    
    QStackedLayout* m_layout;
    DynamicControlList* m_controls;
    QWidget* m_summaryWidget;
    
};

}
#endif
