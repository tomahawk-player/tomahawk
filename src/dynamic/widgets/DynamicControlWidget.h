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

class QStackedLayout;
class QEvent;
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
    explicit DynamicControlWidget( const dyncontrol_ptr& control, bool showPlus = false, bool showMinus = false, bool showCollapse = false, QWidget* parent = 0);
    virtual ~DynamicControlWidget();
    
    void setShowPlusButton( bool show );
    void setShowMinusButton( bool show );
    void setShowCollapseButton( bool show );
       
    virtual void paintEvent(QPaintEvent* );
    virtual void enterEvent(QEvent* );
    virtual void leaveEvent(QEvent* );
    
    dyncontrol_ptr control() const;
    
signals:
    void addNewControl();
    void collapse();
    void removeControl();
    
private slots:
    void typeSelectorChanged( QString );
    
private:
    QToolButton* initButton();
    QWidget* createDummy( QWidget* fromW );
    
    bool m_showPlus;
    bool m_showMinus;
    bool m_showCollapse;
    // i hate qlayout
    QStackedLayout* m_plusL;
    QToolButton* m_plusButton;
    QToolButton* m_minusButton;
    QStackedLayout* m_collapseL;
    QToolButton* m_collapseButton;
    
    dyncontrol_ptr m_control;
    QComboBox* m_typeSelector;
    QHBoxLayout* m_layout;
};
    
};

#endif

class QPaintEvent;
