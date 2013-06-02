/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef LOCALCONFIGSTORAGE_H
#define LOCALCONFIGSTORAGE_H

#include "ConfigStorage.h"

namespace Tomahawk
{

namespace Accounts
{

class LocalConfigStorage : public ConfigStorage
{
    Q_OBJECT
public:
    explicit LocalConfigStorage( QObject* parent = 0 );

    QString id() const { return "localconfigstorage"; }

    QStringList accountIds() const;

    virtual void save( const QString& accountId, const Account::Configuration& cfg );
    virtual void load( const QString& accountId, Account::Configuration& cfg );
    virtual void remove( const QString& accountId );

private:
    const QString m_credentialsServiceName;
    QStringList m_accountIds;

    static LocalConfigStorage* s_instance;
};

} //namespace Accounts

} //namespace Tomahawk

#endif // LOCALCONFIGSTORAGE_H
