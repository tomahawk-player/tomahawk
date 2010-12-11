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

#ifndef DYNAMIC_CONTROL_WIDGET_H
#define DYNAMIC_CONTROL_WIDGET_H

#include <QWidget>

#include "tomahawk/typedefs.h"

class QToolButton;
class QHBoxLayout;
class QComboBox;
class QLabel;

namespace Tomahawk
{

/**
* This widget holds one horizontal control attached to a dynamic playlist. It's a container more than anything.
*/
class DynamicControlWidget : public QWidget
{
    Q_OBJECT 
public:
    explicit DynamicControlWidget( const dyncontrol_ptr& control, bool showPlus = false, QWidget* parent = 0);
    virtual ~DynamicControlWidget();
    
    void setShowPlusButton( bool show );
    bool showPlusButton() const;
    
    virtual void paintEvent(QPaintEvent* );
private slots:
    void typeSelectorChanged( QString );
    
private:
    bool m_showPlus;
    QToolButton* m_plusButton;
    dyncontrol_ptr m_control;
    QComboBox* m_typeSelector;
    QHBoxLayout* m_layout;
};
    
};

#endif

class QPaintEvent;
