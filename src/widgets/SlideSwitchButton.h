/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
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

#ifndef SLIDESWITCHBUTTON_H
#define SLIDESWITCHBUTTON_H

#include <QPushButton>
#include <QColor>
#include <QPixmap>
#include <QFont>

class SlideSwitchButton : public QPushButton
{
    Q_OBJECT
public:
    explicit SlideSwitchButton( QWidget* parent = 0 );
    explicit SlideSwitchButton( const QString& checkedText,
                                const QString& uncheckedText,
                                QWidget* parent = 0 );

    void init();

    virtual QSize sizeHint();
    virtual QSize minimumSizeHint() { return sizeHint(); }

    //the back check-state cannot be changed by the user, only programmatically
    //to notify that the user-requested operation has completed
    void setBackChecked( bool state );
    bool backChecked() const;

    void setKnobX( double x ) { m_knobX = x; repaint(); }
    double knobX() const { return m_knobX; }
    Q_PROPERTY( double knobX READ knobX WRITE setKnobX )

    void setBaseColor( const QColor& color ) { m_baseColor = color; repaint(); }
    QColor baseColor() const { return m_baseColor; }
    Q_PROPERTY( QColor baseColor READ baseColor WRITE setBaseColor )

protected:
    void paintEvent( QPaintEvent* event );

private slots:
    void onCheckedStateChanged();

private:
    QString m_checkedText;
    QString m_uncheckedText;
    QColor m_baseColor;
    QColor m_textColor;
    QColor m_backUncheckedColor;
    QColor m_backCheckedColor;
    QFont m_textFont; //needed for sizeHint
    bool m_backChecked;
    double m_knobX;
};

#endif // SLIDESWITCHBUTTON_H
