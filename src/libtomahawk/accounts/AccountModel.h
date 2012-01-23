/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

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


#ifndef SIPMODEL_H
#define SIPMODEL_H

#include "dllmacro.h"
#include "sip/SipPlugin.h"

#include <QAbstractListModel>
#include <QModelIndex>
#include <QStringList>

namespace Tomahawk
{
namespace Accounts
{

class Account;

class DLLEXPORT AccountModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        AccountName = Qt::UserRole + 15,
        AccountIcon = Qt::UserRole + 16,
        AccountTypeRole = Qt::UserRole + 19,
        ConnectionStateRole = Qt::UserRole + 20,
        HasConfig = Qt::UserRole + 21,
        ErrorString = Qt::UserRole + 22,
        AccountData = Qt::UserRole + 23 // raw plugin
    };

    explicit AccountModel( QObject* parent = 0 );
    virtual ~AccountModel();

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

private slots:
    void accountAdded( Tomahawk::Accounts::Account* p );
    void accountRemoved( Tomahawk::Accounts::Account* p );
    void accountStateChanged( Tomahawk::Accounts::Account::ConnectionState );
};

}

}

#endif // SIPMODEL_H
