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

#ifndef DYNAMIC_SETUP_WIDGET_H
#define DYNAMIC_SETUP_WIDGET_H

#include <QWidget>
// #include <Typedefs.h>
#include "dynplaylist_ptr.h"

class QPropertyAnimation;
class QPaintEvent;
class QHBoxLayout;
class QSpinBox;
class QPushButton;
class QLabel;
class ReadOrWriteWidget;
class QLabel;

namespace Tomahawk
{

/**
 * Widget used to choose a type of dynamic playlist, and to set the number/generate if it's a static one.
 */
class DynamicSetupWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )
public:
    DynamicSetupWidget( const Tomahawk::dynplaylist_ptr& playlist, QWidget* parent = 0 );
    virtual ~DynamicSetupWidget();

    void setPlaylist( const dynplaylist_ptr& playlist );

    qreal opacity() const { return m_opacity; }
    void setOpacity( qreal opacity );

    virtual void paintEvent( QPaintEvent* );

public slots:
    void fadeIn();
    void fadeOut();

signals:
    void generatePressed( int num );
    void typeChanged( const QString& playlistType );

private slots:
    void generatePressed( bool );

private:
    dynplaylist_ptr m_playlist;

    QLabel* m_headerText;
    QHBoxLayout* m_layout;
    ReadOrWriteWidget* m_generatorCombo;
    QLabel* m_logo;
    QPushButton* m_generateButton;
    QSpinBox* m_genNumber;

    QPropertyAnimation* m_fadeAnim;
    qreal m_opacity;
};

};

#endif
