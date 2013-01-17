/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011-2012  Leo Franchi <lfranchi@kde.org>
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

#ifndef ACCOUNTDELEGATE_H
#define ACCOUNTDELEGATE_H

#include "DllMacro.h"
#include <QStyledItemDelegate>
#include "accounts/AccountModel.h"


#ifdef Q_OS_MAC
#define ACCOUNT_DELEGATE_ROW_HEIGHT_MULTIPLIER 4.9
#else
#define ACCOUNT_DELEGATE_ROW_HEIGHT_MULTIPLIER 5.7
#endif


class AnimatedSpinner;

namespace Tomahawk
{
namespace Accounts
{

class Account;

class DLLEXPORT AccountDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    AccountDelegate( QObject* parent = 0);

    virtual void paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual QSize sizeHint ( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

public slots:
    void startInstalling( const QPersistentModelIndex& idx );
    void doneInstalling ( const QPersistentModelIndex& idx );
    void errorInstalling ( const QPersistentModelIndex& idx );

    void doUpdateIndex( const QPersistentModelIndex& idx );

protected:
    virtual bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

signals:
    void update( const QModelIndex& idx );
    void openConfig( Tomahawk::Accounts::Account* );
    void openConfig( Tomahawk::Accounts::AccountFactory* );

private slots:
    void doUpdateIndexWithAccount( Tomahawk::Accounts::Account* account );

private:
    void drawRoundedButton( QPainter* painter, const QRect& buttonRect, bool red = false ) const;
    // Returns new left edge
    int drawStatus( QPainter* painter, const QPointF& rightTopEdge, Account* acct, bool drawText = false ) const;
    void drawCheckBox( QStyleOptionViewItemV4& opt, QPainter* p, const QWidget* w ) const;
    void drawConfigWrench( QPainter* painter, QStyleOptionViewItemV4& option, QStyleOptionToolButton& topt ) const;
    // returns new left edge
    int drawAccountList( QPainter* painter, QStyleOptionViewItemV4& option, const QList< Account* > accounts, int rightEdge ) const;

    QRect checkRectForIndex( const QStyleOptionViewItem &option, const QModelIndex &idx ) const;

    int m_hoveringOver;
    QPersistentModelIndex m_hoveringItem, m_configPressed;
    mutable QHash< QPersistentModelIndex, QRect > m_cachedButtonRects;
    mutable QHash< QPersistentModelIndex, QRect > m_cachedStarRects;
    mutable QHash< QPersistentModelIndex, QRect > m_cachedConfigRects;
    mutable QHash< QPersistentModelIndex, QSize > m_sizeHints;
    mutable QHash< QPersistentModelIndex, AnimatedSpinner* > m_loadingSpinners;
    mutable QHash< Account*, AnimatedSpinner* > m_connectingSpinners;
    mutable int m_accountRowHeight;

    mutable QAbstractItemModel* m_model;
};

}

}

#endif // ACCOUNTDELEGATE_H
