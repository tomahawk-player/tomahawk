/*
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef ACCOUNTDELEGATE_H
#define ACCOUNTDELEGATE_H

#include "configdelegatebase.h"
#include "accounts/AccountModel.h"

namespace Tomahawk
{
namespace Accounts
{

class Account;

class AccountDelegate : public ConfigDelegateBase
{
    Q_OBJECT
public:
    AccountDelegate( QObject* parent = 0);

    virtual void paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual bool editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );
    virtual QSize sizeHint ( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual QRect checkRectForIndex( const QStyleOptionViewItem &option, const QModelIndex &idx, int role ) const;
    virtual QRect configRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const;

private slots:
    void askedForEdit( const QModelIndex& idx );

signals:
    void update( const QModelIndex& idx );
    void openConfig( Tomahawk::Accounts::Account* );

private:
    void paintTopLevel( QPainter* painter, const QStyleOptionViewItemV4& option, const QModelIndex& index ) const;
    void paintChild( QPainter* painter, const QStyleOptionViewItemV4& option, const QModelIndex& index ) const;
    void drawRoundedButton( QPainter* painter, const QRect& buttonRect ) const;

    QMap< QString, QPixmap > m_cachedIcons;
    QPixmap m_offlineIcon, m_onlineIcon, m_defaultCover, m_onHoverStar, m_ratingStarPositive, m_ratingStarNegative;

    int m_widestTextWidth;
    int m_hoveringOver;
    QPersistentModelIndex m_hoveringItem;
    mutable QHash< QPersistentModelIndex, QRect > m_cachedButtonRects;
    mutable QHash< QPersistentModelIndex, QRect > m_cachedStarRects;
};

}
}

#endif // ACCOUNTDELEGATE_H
