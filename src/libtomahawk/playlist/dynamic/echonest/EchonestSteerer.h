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

class QToolButton;
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
    
public slots:
    void resetSteering( bool automatic = false );
    
signals:
    void steerField( const QString& field );
    void steerDescription( const QString& desc );
    
    void resized();
private slots:
    void changed();
    
    void resizeFrame( int );
    
private:
    QToolButton* initButton( QWidget* parent );
    
    QHBoxLayout* m_layout;
    
    QComboBox* m_amplifier;
    QComboBox* m_field;
    
    QLineEdit* m_description;
    
    // text on the left
    QVBoxLayout* m_textL;
    QLabel* m_steerTop;
    QLabel* m_steerBottom;
    
    // icons on the right
    QToolButton* m_reset;
   
    // animations
    QTimeLine m_resizeAnim;
    bool m_expanding;
};
    
};

#endif
