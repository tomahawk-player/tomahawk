/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#ifndef ACCOUNTSPOPUPWIDGET_H
#define ACCOUNTSPOPUPWIDGET_H

#include <QWidget>

class QVBoxLayout;

class AccountsPopupWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AccountsPopupWidget( QWidget* parent = 0 );

    void setWidget( QWidget* widget );
    void anchorAt( const QPoint &p );

signals:
    void hidden();
    
protected:
    virtual void paintEvent( QPaintEvent* );
    virtual void focusOutEvent( QFocusEvent* );
    virtual void hideEvent( QHideEvent* );
    
private:
    QVBoxLayout* m_layout;
    QWidget* m_widget;
};

#endif // ACCOUNTSPOPUPWIDGET_H
