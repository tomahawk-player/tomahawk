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

#ifndef BREADCRUMBBAR_H
#define BREADCRUMBBAR_H


#include "breadcrumbbuttonbase.h"

#include <QModelIndex>
#include <QWidget>
#include <QLinkedList>
#include <QIcon>

class QAbstractItemModel;
class QItemSelectionModel;
class QAbstractItemModel;
class QAbstractProxyModel;
class QHBoxLayout;
class QItemSelectionModel;
class QResizeEvent;

/**
 * \brief A breadcrumb view for a QAbstractItemModel
 *
 * This a lean Breadcrumb navigation bar that supports the following features:
 *	- a QAbstractItemModel data source for MV goodness
 *	- client provided crumb button widgets (to use your own style and behavior)
 *	- client provided crumb animation [optional]
 *	- client provided root item (icon or text) [optional]
 *
 */
class BreadcrumbBar : public QWidget
{
    Q_OBJECT

public:
    /**
     *	\brief breadcrumb bar constructor
     *	\param buttonFactory the button factory to instantiate bread crumbs from
     */
    BreadcrumbBar(BreadcrumbButtonFactory *buttonFactory, QWidget *parent = 0);
    /**
     *	\brief breadcrumb bar constructor
     *	You must set the button factory using BreadcrumbBar::setButtonFactory
     */
    BreadcrumbBar(QWidget *parent = 0);

    virtual ~BreadcrumbBar();

    /**
     * \brief sets the button factory to use to create buttons
     * \param buttonFactory the button factory
     */
    void setButtonFactory(BreadcrumbButtonFactory *buttonFactory);
    BreadcrumbButtonFactory* buttonFactory() const;

    /**
     * \brief Set the icon that should be displayed at the root
     * \param icon the icon
     */
    void setRootIcon(const QIcon &icon);
    /**
     * \brief Set the text that should be displayed at the root
     * \param test the text
     */
    void setRootText(const QString &text);

    /**
     * \brief Set whether the crumb animation should be used (default: false)
     */
    void setUseAnimation(bool use);
    bool useAnimation() const;

    /**
     *	\brief set the item model to use as a data source
     *	\param model the item model
     *
     *	Although the item model can be of any structure, the breadcrumb bar
     *	works best when the model's structure is a tree.
     */
    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model();

    /**
     *	\brief set the selection model used to determine the crumb items
     *	\param selectionModel the selection model
     *
     *  The selection model is just as important as your model. In fact your
     *  model can be of any structure (even not-tree-like) as long as your
     *  selection model understands what it means for an item to be a crumb,
     *  and knows how to select it.
     *
     *  If you have a standard tree model, you probably should use
     *  KBreadCrumbSelectionModel which encapsulates the concept of "When an
     *  item is selected, make a selection which includes its parent and
     *  ancestor items until the top is reached".
     *
     *  \sa See Stephen Kelley's blog post here http://steveire.wordpress.com/2010/04/21/breadcrumbs-for-your-view
     */
    void setSelectionModel(QItemSelectionModel *selectionModel);
    QItemSelectionModel *selectionModel();

    /**
     *	\brief get the index of the currently active item
     *	\return the active index
     */
    QModelIndex currentIndex();

    /**
     *	\brief used by crumbs to notify that the current index has changed
     *	\param index the new current index
     */
    void currentChangedTriggered(QModelIndex const& index);

protected:
    /**
     *	\brief append a crumb widget
     *	\param button the crumb button to add
     *	\param stretch widget stretch factor
     *	Respects the useAnimation() setting.
     */
    void appendButton(BreadcrumbButtonBase *button, int stretch = 0);

    /**
     *	\brief deletes a crumb from the bar
     *	\param button the crumb button to delete
     *	Respects the useAnimation() setting.
     */
    void deleteButton(BreadcrumbButtonBase *button);

    /**
     * \brief collapses crumbs when there isnt enough room for all of them
     * Starts hiding from the left until we can fit all the buttons.
     */
    void collapseButtons();
    /**
     * \brief reimpl from QWidget
     */
    void resizeEvent ( QResizeEvent * event );


protected slots:

    /**
     * \brief The current index has changed in the selection model
     * When the selection model changes, we get notified here
     */
    void currentIndexChanged();

    /**
     *	\brief Recreate the button bar from the selection model
     */
    void updateButtons();
    /**
     *	\brief clear breadcrumb buttons
     */
    void clearButtons();

    /**
     * Called when the delete animation finishes so we can delete the button
     * object.
     */
    void deleteAnimationFinished();


private:
    BreadcrumbButtonFactory *m_buttonFactory; /*!< Factory used to create new crumbs */
    QAbstractItemModel *m_model; /*!< The source model */
    QItemSelectionModel *m_selectionModel; /*!< The selection model */
    QHBoxLayout *m_layout; /*!< The layout holding out crumb buttons */
    QLinkedList<BreadcrumbButtonBase*> m_navButtons; /*< Our list of crumbs! */
    QIcon m_rootIcon; /*!< The root icon */
    QString m_rootText; /*!< The root text */
    bool m_useAnimation; /*<!< Whether we should animate the transition or not */
};


#endif // BREADCRUMBBAR_H
