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

#ifndef CONFIGSTORAGE_H
#define CONFIGSTORAGE_H

#include "TomahawkSettings.h"
#include "Account.h"

#include <QObject>

namespace Tomahawk
{

namespace Accounts
{

class DLLEXPORT ConfigStorage : public QObject
{
    Q_OBJECT
public:
    explicit ConfigStorage( QObject* parent = 0 );

    virtual ~ConfigStorage();

    virtual void init() = 0;

    virtual QString id() const = 0;

    virtual QStringList accountIds() const = 0;

    virtual void save( const QString& accountId, const Account::Configuration& cfg ) = 0;
    virtual void load( const QString& accountId, Account::Configuration& cfg ) = 0;
    virtual void remove( const QString& accountId ) = 0;

signals:
    void ready(); //emit this when done with whatever it is that needs to be initialized
};

} //namespace Accounts
} //namespace Tomahawk

Q_DECLARE_INTERFACE( Tomahawk::Accounts::ConfigStorage, "tomahawk.ConfigStorage/1.0" )

#endif // CONFIGSTORAGE_H
