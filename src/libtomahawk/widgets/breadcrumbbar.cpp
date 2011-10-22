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

#include "breadcrumbbar.h"
#include "breadcrumbbuttonbase.h"

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QPushButton>

#include "utils/logger.h"


BreadcrumbBar::BreadcrumbBar( BreadcrumbButtonFactory *buttonFactory, QWidget *parent )
    : QWidget( parent )
    , m_buttonFactory( buttonFactory )
    , m_model( 0 )
    , m_selectionModel( 0 )
    , m_layout( new QHBoxLayout( this ) )
    , m_useAnimation( false )

{
    m_layout->setSpacing( 0 );
    m_layout->setMargin( 0 );
    m_layout->setAlignment( Qt::AlignLeft );

    setAutoFillBackground( false );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    setLayoutDirection( Qt::LeftToRight );
    setLayout( m_layout );
    setMinimumWidth( 100 );
    show();
}


BreadcrumbBar::BreadcrumbBar( QWidget *parent )
    : QWidget( parent )
    , m_buttonFactory( 0 )
    , m_model( 0 )
    , m_selectionModel( 0 )
    , m_layout( new QHBoxLayout( this ) )
    , m_useAnimation( false )

{
    m_layout->setSpacing( 0 );
    m_layout->setMargin( 0 );
    m_layout->setAlignment( Qt::AlignLeft );

    setAutoFillBackground( false );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    setLayoutDirection( Qt::LeftToRight );
    setLayout( m_layout );
    setMinimumWidth( 100 );
    show();
}


BreadcrumbBar::~BreadcrumbBar()
{
}


void BreadcrumbBar::setButtonFactory(BreadcrumbButtonFactory *buttonFactory)
{
    tDebug( LOGVERBOSE ) << "Breadcrumbbar:: got me some button factory!";
    m_buttonFactory = buttonFactory;
}


BreadcrumbButtonFactory* BreadcrumbBar::buttonFactory() const
{
    return m_buttonFactory;
}


void BreadcrumbBar::appendButton(BreadcrumbButtonBase *widget, int stretch)
{
    m_layout->insertWidget(m_layout->count(), widget, stretch);
    if( !m_useAnimation )
        return; //we're done here.

    // A nifty trick to force the widget to calculate its position and geometry
    //widget->setAttribute(Qt::WA_DontShowOnScreen);
    widget->show();
    //widget->setAttribute(Qt::WA_DontShowOnScreen, false);

    if( m_navButtons.size() > 0 ) {
        QWidget* neighbor = m_layout->itemAt(m_layout->count()-2)->widget();
        QPropertyAnimation *animation = new QPropertyAnimation(widget,"pos");
        animation->setDuration(300);
        animation->setStartValue(neighbor->pos());
        animation->setEndValue(widget->pos());
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}


void BreadcrumbBar::deleteAnimationFinished()
{
    QPropertyAnimation *anim = qobject_cast<QPropertyAnimation*>(sender());

    if( !anim )
        return;
    QObject *obj = anim->targetObject();
    obj->deleteLater();
    anim->deleteLater();
}


void BreadcrumbBar::deleteButton(BreadcrumbButtonBase *widget)
{
    widget->hide();
    widget->deleteLater();
    return; // all done here

    // Don't animate on delete. We expand a child recursively until it has no more children---this makes
    // the deleting and creating animations overlap.

//     int index = m_layout->indexOf(widget);
//     if( index != 0 && m_navButtons.size() > 0 ) {
//         QWidget* neighbor = m_layout->itemAt(index-1)->widget();
//         QPropertyAnimation *animation = new QPropertyAnimation(widget,"pos");
//         m_layout->removeWidget(widget);
//         connect(animation, SIGNAL(finished()), SLOT(deleteAnimationFinished()));
//         animation->setDuration(300);
//         animation->setStartValue(widget->pos());
//         animation->setEndValue(neighbor->pos());
//         animation->start();
//     } else {
//         widget->hide();
//         widget->deleteLater();
//     }
}


void BreadcrumbBar::updateButtons()
{
    tDebug( LOGVERBOSE ) << "Breadcrumbbar:: updateButtons" << m_buttonFactory << m_selectionModel ;
    if ( m_selectionModel )
        tDebug( LOGVERBOSE ) <<"Breadcrumbbar:: update buttoms current index"<< m_selectionModel->currentIndex().isValid();
    if ( !m_buttonFactory || !m_selectionModel || !m_selectionModel->currentIndex().isValid() )
    {
        tDebug( LOGVERBOSE ) << "Breadcrumb:: updatebuttons failed!";
        return;
    }

    QLinkedList<BreadcrumbButtonBase*>::iterator it = m_navButtons.begin();
    QLinkedList<BreadcrumbButtonBase*>::const_iterator const itEnd = m_navButtons.end();
    bool createButton = false;

    QModelIndex index = m_selectionModel->currentIndex();
    QList<QModelIndex> indexes;
    while (index.parent().isValid())
    {
        indexes.prepend(index);
        index = index.parent();
    }
    tDebug( LOGVERBOSE ) << "BreadcrumbBar::updateButtons:: " << index.data().toString();
    indexes.prepend(index);

    int count = indexes.size(), i = 0;
    foreach (index, indexes)
    {
        createButton = (it == itEnd);
        bool isLastButton = (++i == count);

        QString const dirName = index.data().toString();
        BreadcrumbButtonBase *button = 0;
        if (createButton)
        {
            button = m_buttonFactory->newButton(index, this);
            appendButton(button);
            button->setActive(isLastButton);
            m_navButtons.append(button);
        }
        else
        {
            button = *it;
            button->setIndex(index);
            button->setActive(isLastButton);
            ++it;
        }
    }

    QLinkedList<BreadcrumbButtonBase*>::Iterator itBegin = it;
    while (it != itEnd)
    {
        deleteButton(*it);
        ++it;
    }
    m_navButtons.erase(itBegin, m_navButtons.end());

    collapseButtons();

    adjustSize();
}


void BreadcrumbBar::collapseButtons()
{
    foreach (BreadcrumbButtonBase *button, m_navButtons) {
        button->show();
    }

    const int desired_width = size().width();
    int current_width = sizeHint().width();

    QLinkedList<BreadcrumbButtonBase*>::iterator it = m_navButtons.begin();
    QLinkedList<BreadcrumbButtonBase*>::const_iterator const itEnd = m_navButtons.end();
    it = m_navButtons.begin();
    while( current_width > desired_width && it != itEnd ) {
        (*it)->hide();
        ++it;
        current_width = sizeHint().width();
    }
}


void BreadcrumbBar::clearButtons()
{
    foreach (BreadcrumbButtonBase *button, m_navButtons)
    {
        button->hide();
        button->deleteLater();
    }
    m_navButtons.clear();
}


void BreadcrumbBar::currentIndexChanged()
{
    updateButtons();
}


void BreadcrumbBar::setRootIcon(const QIcon &icon)
{

    m_rootIcon = icon;
    QPushButton* button = new QPushButton(icon, "", this);
    button->setFlat(true);
    button->setStyleSheet( "QPushButton{ background-color: transparent; border: none; width:16px; height:16px;}" );
    m_layout->insertWidget(0, button);
    m_layout->insertSpacing(0,5);
    m_layout->insertSpacing(2,5);
    connect(button, SIGNAL(clicked()), this, SIGNAL(rootClicked()));
}


void BreadcrumbBar::setRootText(const QString &text)
{
    //TODO: implement this
    m_rootText = text;
    /*QLabel *label= new QLabel(this);
    label->setPixmap(icon.pixmap(16,16));
    m_layout->insertWidget(0, label);
    m_layout->insertSpacing(0,5);
    m_layout->insertSpacing(2,5);*/
}


void BreadcrumbBar::setUseAnimation(bool use)
{
    m_useAnimation = use;
}


bool BreadcrumbBar::useAnimation() const
{
    return m_useAnimation;
}


void BreadcrumbBar::setModel(QAbstractItemModel *model)
{
    m_model = model;
    updateButtons();
}


QAbstractItemModel* BreadcrumbBar::model()
{
    return m_model;
}


void BreadcrumbBar::setSelectionModel(QItemSelectionModel *selectionModel)
{
    m_selectionModel = selectionModel;
    m_selectionModel->setCurrentIndex(m_model->index(0,0), QItemSelectionModel::SelectCurrent);
    connect(m_selectionModel,
        SIGNAL(currentChanged(QModelIndex const&, QModelIndex const&)),
        this, SLOT(currentIndexChanged()));
    updateButtons();
}


QItemSelectionModel* BreadcrumbBar::selectionModel()
{
    return m_selectionModel;
}


QModelIndex BreadcrumbBar::currentIndex()
{
    return m_selectionModel->currentIndex();
}


void BreadcrumbBar::currentChangedTriggered(QModelIndex const& index)
{
    Q_ASSERT(m_selectionModel);
    m_selectionModel->setCurrentIndex( index, QItemSelectionModel::SelectCurrent);
    emit currentIndexChanged(index);
}


void BreadcrumbBar::resizeEvent ( QResizeEvent * event )
{
    Q_UNUSED( event );
    collapseButtons();
}
