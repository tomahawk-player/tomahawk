/****************************************************************************************
 * Copyright (c) 2011 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef ECHONEST_STEERER_H
#define ECHONEST_STEERER_H

#include <QWidget>
#include <QTimeLine>

class QLabel;
class QComboBox;
class QVBoxLayout;
class QLineEdit;
class QHBoxLayout;

namespace Tomahawk 
{
    
class EchonestSteerer : public QWidget
{
    Q_OBJECT
public:
    EchonestSteerer( QWidget*  parent = 0 );
    
    virtual void paintEvent(QPaintEvent* );
    
signals:
    void steerField( const QString& field );
    void steerDescription( const QString& desc );
    
    void resized();
private slots:
    void changed();
    
    void resizeFrame( int );
    void resizeFinished();
    
private:
    QHBoxLayout* m_layout;
    QLabel* m_steerTop;
    QLabel* m_steerBottom;
    
    QComboBox* m_amplifier;
    QComboBox* m_field;
    
    QLineEdit* m_description;
    
    QVBoxLayout* m_textL;
    
    // animations
    QTimeLine m_resizeAnim;
    bool m_expanding;
};
    
};

#endif
