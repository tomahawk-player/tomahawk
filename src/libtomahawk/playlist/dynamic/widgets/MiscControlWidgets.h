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

#ifndef MISC_CONTROL_WIDGETS_H
#define MISC_CONTROL_WIDGETS_H

#include <QWidget>

class QLabel;
class QSlider;
namespace Tomahawk
{
    
class LabeledSlider : public QWidget
{
    Q_OBJECT
public:
    explicit LabeledSlider( const QString& leftT, const QString& rightT, QWidget* parent = 0 );
    
    QSlider* slider() { return m_slider; }
    
private:
    QSlider* m_slider;
    QLabel* m_leftLabel;
    QLabel* m_rightLabel;
};
    
    
}

#endif
