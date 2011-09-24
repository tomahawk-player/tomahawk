/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QtCore/QObject>

#include "typedefs.h"
#include "dllmacro.h"
#include "infosystem/infosystem.h"
#include "sip/SipPlugin.h"
#include "account.h"

namespace Tomahawk
{

namespace Accounts
{

class DLLEXPORT AccountManager : public QObject
{
    Q_OBJECT
    
public:
    explicit AccountManager();
    virtual ~AccountManager();

    QStringList findPluginFactories();
    void loadPluginFactories( const QStringList &paths );

    void loadFromConfig();
    void loadPluginFactory( const QString &path );
    void addAccountPlugin( Account* account );
    Account* loadPlugin( const QString &pluginId );
    QString factoryFromId( const QString& pluginId ) const;
    
    //QSet< Account > getAccounts( Tomahawk::Accounts::AccountType type );

private:
    QSet< Account* > m_accounts;
    QHash< QString, AccountFactory* > m_accountFactories;
    
};

};

};

#endif