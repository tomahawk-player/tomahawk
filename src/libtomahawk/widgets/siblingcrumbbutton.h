/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Casey Link <unnamedrambler@gmail.com>
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

#ifndef SIBLINGCRUMBBUTTON_H
#define SIBLINGCRUMBBUTTON_H

#include "breadcrumbbuttonbase.h"
#include "breadcrumbbar.h"

#include "combobox.h"

#include <QList>
#include <QModelIndex>
#include <QPointer>
#include <QSize>
#include <QMenu>

/**
 * \brief A factory for sibling crumb buttons
 */
class SiblingCrumbButtonFactory : public BreadcrumbButtonFactory
{

    public:
        SiblingCrumbButtonFactory(){}

        virtual ~SiblingCrumbButtonFactory(){}
        virtual BreadcrumbButtonBase* newButton(QModelIndex index, BreadcrumbBar *parent);
};

/**
 * \brief A crumb button implementation where the dropdowns show sibling items
 * Unlike most crumb buttons, this one shows a list of sibling items (as
 * opposed to child items). This is desireable in certain circumstances.
 */
class SiblingCrumbButton : public BreadcrumbButtonBase
{
    Q_OBJECT

public:
    SiblingCrumbButton(QModelIndex index, BreadcrumbBar *parent);

    void setIndex(QModelIndex index);
    QModelIndex index() const;
    void setActive(bool active);
    bool isActive() const;
    virtual QSize sizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent *event);

private slots:
    void fillCombo();
    void comboboxActivated(int i);
    void activateSelf();

private:
    bool hasChildren() const;

    QModelIndex m_index; /*!< our current index */
    ComboBox *m_combo; /*!< our combobox! */

};

#endif // SIBLINGCRUMBBUTTON_H
