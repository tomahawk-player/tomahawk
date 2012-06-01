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

#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QAbstractItemView>
#include <QTimer>

#include "DllMacro.h"

class DLLEXPORT OverlayWidget : public QWidget
{
Q_OBJECT
Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )

public:
    OverlayWidget( QWidget* parent );
    OverlayWidget( QAbstractItemView* parent );
    ~OverlayWidget();

    qreal opacity() const { return m_opacity; }
    void setOpacity( qreal opacity );

    QString text() const { return m_text; }
    void setText( const QString& text );

    bool shown() const;

public slots:
    void show( int timeoutSecs = 0 );
    void hide();

protected:
//    void changeEvent( QEvent* e );
    void paintEvent( QPaintEvent* event );
    
private slots:
    void onViewChanged();
    void onViewModelChanged();

private:
    void init();

    QString m_text;
    qreal m_opacity;

    QWidget* m_parent;
    QAbstractItemView* m_itemView;

    QTimer m_timer;
};

#endif // OVERLAYWIDGET_H
