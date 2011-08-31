/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Casey Link <unnamedrambler@gmail.com>
 *   Copyright (C) 2004-2011 Glenn Van Loon, glenn@startupmanager.org
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

#ifndef BREADCRUMBBUTTONBASE_P_H
#define BREADCRUMBBUTTONBASE_P_H

#include <QPushButton>
#include <QModelIndex>

class BreadcrumbBar;
class BreadcrumbButtonBase;

/**
 * \brief an abstract factory class to create crumb buttons
 * Subclass this class and make it return bread crumb buttons of your type.
 */
class BreadcrumbButtonFactory {

    public:
        BreadcrumbButtonFactory(){}

        virtual ~BreadcrumbButtonFactory(){}

        /**
         * \brief instantiates a new bread crumb button
         * \param index the initial index this crumb should hold
         * \param parent the breadcrumb bar this button will belong to
         * \returns a new bread crumb button allocated on the heap.
         *
         * This method can be as simple as:
         * \code
         *      return new MyBreadCrumbButton(index, parent);
         * \endcode
         *
         */
        virtual BreadcrumbButtonBase* newButton(QModelIndex index, BreadcrumbBar *parent) = 0;

};

/**
 *	\brief The button base class for the BreadcrumbBar
 * Re-implement this to provide your own crumb buttons. Don't forget to supply
 * a BreadcrumbButtonFactory as well.
 */
class BreadcrumbButtonBase : public QPushButton
{
    Q_OBJECT

public:
    explicit BreadcrumbButtonBase(BreadcrumbBar *parent);
    virtual ~BreadcrumbButtonBase();

    /**
     *	\brief retrieve the breadcrumb bar that this button belongs to
     *	\return the parent breadcrumb bar
     */
    BreadcrumbBar* breadcrumbBar() const;

    /**
     *	\brief set the model item that this button represents
     *	\param index the index of the model items to display
     */
    virtual void setIndex(QModelIndex index) = 0;
    virtual QModelIndex index() const = 0;

    /**
     *	\brief sets whether this button is active or not
     *	\param active true for active, false for inactive
     *	You could, for example, make the active button bold.
     */
    virtual void setActive(bool active) = 0;
    virtual bool isActive() const = 0;

private:
    BreadcrumbBar *m_breadcrumbBar;
};

#endif // BREADCRUMBBUTTONBASE_P_H
