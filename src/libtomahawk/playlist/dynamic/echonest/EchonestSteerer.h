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

#ifndef ECHONEST_STEERER_H
#define ECHONEST_STEERER_H

#include <QWidget>
#include <QTimeLine>

class QPropertyAnimation;
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
    Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )

public:
    EchonestSteerer( QWidget*  parent = 0 );

    virtual void paintEvent(QPaintEvent* );

public slots:
    void applySteering();
    void resetSteering( bool automatic = false );

    void fadeIn();
    void fadeOut();
    qreal opacity() const { return m_opacity; }
    void setOpacity( qreal opacity );
signals:
    void steerField( const QString& field );
    void steerDescription( const QString& desc );
    void reset();

    void resized();

    // interface to DynamicWidget
    void steeringChanged();
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
    QToolButton* m_apply;
    QToolButton* m_reset;

    // animations
    QTimeLine m_resizeAnim;
    bool m_expanding;

    QPropertyAnimation* m_fadeAnim;
    qreal m_opacity;
};

};

#endif
