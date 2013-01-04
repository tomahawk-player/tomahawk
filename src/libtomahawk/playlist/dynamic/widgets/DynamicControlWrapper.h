/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef DYNAMIC_CONTROL_WRAPPER_H
#define DYNAMIC_CONTROL_WRAPPER_H

#include <QWidget>
#include <QPointer>

#include "Typedefs.h"

class QGridLayout;
class QStackedLayout;
class QEvent;
class QToolButton;
class QHBoxLayout;
class QComboBox;
class QLabel;;

namespace Tomahawk
{

/**
* This abstraction object manages the widgets for 1 dynamic playlist control, laid out in the desired layout
*/
class DynamicControlWrapper : public QObject
{
    Q_OBJECT
public:
    explicit DynamicControlWrapper( const dyncontrol_ptr& control, QGridLayout* layout, int row, QWidget* parent = 0 );
    virtual ~DynamicControlWrapper();

//     virtual void enterEvent(QEvent* );
//     virtual void leaveEvent(QEvent* );

    dyncontrol_ptr control() const;

    void removeFromLayout();


    static QToolButton* initButton( QWidget* parent );
    static QWidget* createDummy( QWidget* fromW, QWidget* parent );
signals:
    void collapse();
    void removeControl();
    void changed();

private slots:
    void typeSelectorChanged( const QString& selectedType, bool firstLoad = false );

private:
    QWidget* m_parent;
    int m_row;
    QStackedLayout* m_plusL;
    QToolButton* m_minusButton;

    dyncontrol_ptr m_control;
    QComboBox* m_typeSelector;
    QPointer<QWidget> m_matchSelector;
    QPointer<QWidget> m_entryWidget;
    QPointer<QGridLayout> m_layout;
};

};

#endif

class QPaintEvent;

class QMouseEvent;
