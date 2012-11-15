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

#ifndef DYNAMIC_CONTROL_LIST_H
#define DYNAMIC_CONTROL_LIST_H

#include "Typedefs.h"
#include "playlist/dynamic/DynamicPlaylist.h"

#include <QStackedWidget>

class QEvent;
class QGridLayout;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QToolButton;

namespace Tomahawk
{

class DynamicControlWrapper;


/**
 * This widget encapsulates the list of dynamic controls. It can hide or show the controls.
 */

class DynamicControlList : public QWidget
{
    Q_OBJECT
public:
    DynamicControlList( QWidget* parent = 0 );
    explicit DynamicControlList( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls, QWidget* parent = 0 );
    virtual ~DynamicControlList();

    void setControls( const geninterface_ptr& generator, const QList< dyncontrol_ptr >& controls );
    QList< DynamicControlWrapper* > controls() const { return m_controls; }

signals:
    void controlsChanged( bool added );
    void controlChanged( const Tomahawk::dyncontrol_ptr& control );
    void toggleCollapse();

public slots:
    void addNewControl();
    void removeControl();
    void controlChanged();

private:
    void init();

    geninterface_ptr m_generator;

    QGridLayout* m_layout;
    QList< DynamicControlWrapper* > m_controls;

    QHBoxLayout* m_collapseLayout;
    QPushButton* m_collapse;
    QToolButton* m_addControl;

};

};

#endif
