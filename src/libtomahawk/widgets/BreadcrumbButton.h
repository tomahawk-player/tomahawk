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

#ifndef TOMAHAWK_BREADCRUMBBUTTON_H
#define TOMAHAWK_BREADCRUMBBUTTON_H

#include <QWidget>
#include <QModelIndex>

class ComboBox;
class BreadcrumbArrow;
class QPaintEvent;

namespace Tomahawk {

class Breadcrumb;
class BreadcrumbModel;

class BreadcrumbButton : public QWidget
{
    Q_OBJECT
public:
    explicit BreadcrumbButton( Breadcrumb* parent, QAbstractItemModel* model );

    void setParentIndex( const QModelIndex& idx );

    // Which index is currently visible. This is the first, default or last selected
    // calculated immediately after loading, or what the user has selected if he has made
    // a manua;l selection
    QModelIndex currentIndex() const;

protected:
    virtual void paintEvent( QPaintEvent* );
    virtual QSize sizeHint() const;

signals:
    // Some combobox value is changed
    void currentIndexChanged( const QModelIndex& childSelected );

private slots:
    void comboboxActivated( int );

private:
    bool hasChildren() const;

    Breadcrumb* m_breadcrumb;
    QAbstractItemModel* m_model;

    QPersistentModelIndex m_parentIndex;
    QPersistentModelIndex m_curIndex;
    ComboBox* m_combo;
    BreadcrumbArrow* m_arrow;
};

}

#endif // TOMAHAWK_BREADCRUMBBUTTON_H
