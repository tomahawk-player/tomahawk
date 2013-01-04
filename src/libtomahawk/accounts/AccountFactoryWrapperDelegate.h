/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#ifndef ACCOUNTFACTORYWRAPPERDELEGATE_H
#define ACCOUNTFACTORYWRAPPERDELEGATE_H

#include <QStyledItemDelegate>

#define ACCOUNT_ROW_HEIGHT 20

namespace Tomahawk {
namespace Accounts {
class Account;
}
}

class AccountFactoryWrapperDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AccountFactoryWrapperDelegate( QObject* parent = 0 );

    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);

signals:
    void update( const QModelIndex& );

    void openConfig( Tomahawk::Accounts::Account* );
    void removeAccount( Tomahawk::Accounts::Account* );

    void checkOrUncheck( const QModelIndex& row, Tomahawk::Accounts::Account* account, Qt::CheckState newState );

private:
    QModelIndex m_configPressed;

    mutable QHash< QPersistentModelIndex, QRect > m_cachedCheckRects;
    mutable QHash< QPersistentModelIndex, QRect > m_cachedButtonRects;
    mutable QHash< QPersistentModelIndex, QRect > m_cachedConfigRects;
};

#endif // ACCOUNTFACTORYWRAPPERDELEGATE_H
