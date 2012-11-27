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

#ifndef TOMAHAWK_BREADCRUMB_H
#define TOMAHAWK_BREADCRUMB_H

#include <QWidget>
#include <QModelIndex>

class QHBoxLayout;
class QAbstractItemModel;

namespace Tomahawk {

class BreadcrumbButton;

/**
 * A breadcrumb widget for Tomahawk's needs.
 *
 * This breadcrumb operates on a QAIM. It uses a KBreadcrumbSelectionModel to manage the visible
 *  breadcrumb selection.
 * This breadcrumb always expands fully to the deepest child.
 * Selections are remembered when switching to/from in parent nodes
 * Items that have a DefaultRole set will automatically select the default unless the user has
 *  made a previous selection, which is saved in the UserSelection role
 */
class Breadcrumb : public QWidget
{
    Q_OBJECT
public:
    enum ExtraRoles {
        DefaultRole = Qt::UserRole + 1,
        UserSelectedRole = Qt::UserRole + 2,
        ChartIdRole = Qt::UserRole + 3,
        ChartExpireRole = Qt::UserRole + 4
    };

    explicit Breadcrumb( QWidget* parent = 0, Qt::WindowFlags f = 0 );
    virtual ~Breadcrumb();

    void setModel( QAbstractItemModel* model );
    QAbstractItemModel* model() const { return m_model; }

    void setRootIcon( const QPixmap& pm );

protected:
    virtual void paintEvent( QPaintEvent* );

signals:
    void activateIndex( const QModelIndex& idx );

private slots:
    void breadcrumbComboChanged( const QModelIndex& );

private:
    // Takes an index in the selection model to update from (updates from that to all children)
    void updateButtons( const QModelIndex& fromIndex );

    QAbstractItemModel* m_model;
    QPixmap m_rootIcon;

    QHBoxLayout* m_buttonlayout;
    QList<BreadcrumbButton*> m_buttons;
};

}

#endif // TOMAHAWK_BREADCRUMB_H

