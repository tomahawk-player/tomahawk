/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef TOMAHAWK_ACCOUNTS_ACCOUNTMODELNODE_H
#define TOMAHAWK_ACCOUNTS_ACCOUNTMODELNODE_H

#include "Account.h"
#include "AccountManager.h"
#include "ResolverAccount.h"

#include <attica/content.h>

namespace Tomahawk {

namespace Accounts {

/**
 * Node for account tree.
 *
 * Basically a union with possible types:
 * 1) AccountFactory* for accounts that are not unique (jabber, google, twitter)
 * 2) Account* for accounts that are associated with an AccountFactory (children of AccountFactory)
 * 3) Attica::Content for AtticaResolverAccounts (with associated AtticaResolverAccount*) (all synchrotron resolvers)
 * 4) ResolverAccount* for manually added resolvers (from file).
 * 5) AccountFactory* + Account* for factories that are unique
 *
 * These are the top-level items in tree.
 *
 * Top level nodes all look the same to the user. The only difference is that services that have login (and thus
 *  can have multiple logins at once) allow a user to create multiple children with specific login information.
 *  All other top level accounts (Account*, Attica::Content, ResolverAccount*) behave the same to the user, they can
 *  simply click "Install" or toggle on/off.
 *
 */

struct AccountModelNode {
    enum NodeType {
        FactoryType,
        UniqueFactoryType,
        AccountType,
        AtticaType,
        ManualResolverType
    };
    AccountModelNode* parent;
    NodeType type;
    QList< AccountModelNode* > children; // list of children accounts (actually existing and configured accounts)

    /// 1.
    AccountFactory* factory;

    /// 2.
    Account* account;

    /// 3.
    Attica::Content atticaContent;
    AtticaResolverAccount* atticaAccount;

    /// 4.
    ResolverAccount* resolverAccount;

    // Construct in one of four ways. Then access the corresponding members
    explicit AccountModelNode( AccountModelNode* p, AccountFactory* fac ) : parent( p ), type( FactoryType )
    {
        init();
        factory = fac;

        if ( fac->isUnique() )
            type = UniqueFactoryType;

        // Initialize factory nodes with their children
        foreach ( Account* acct,  AccountManager::instance()->accounts() )
        {
            if ( AccountManager::instance()->factoryForAccount( acct ) == fac )
            {
                qDebug() << "Found account for factory:" << acct->accountFriendlyName();
                if ( fac->isUnique() )
                {
                    account = acct;
                    break;
                }
                else
                {
                    new AccountModelNode( this, acct );
                }
            }
        }
    }

    AccountModelNode( AccountModelNode* p, Account* acct ) : parent( p ), type( AccountType )
    {
        init();
        account = acct;
    }

    explicit AccountModelNode( AccountModelNode* p, Attica::Content cnt ) : parent( p ), type( AtticaType )
    {
        init();
        atticaContent = cnt;

        qDebug() << "Creating attica model node for resolver:" << cnt.id();

        foreach ( Account* acct, AccountManager::instance()->accounts( Accounts::ResolverType ) )
        {
            if ( AtticaResolverAccount* resolver = qobject_cast< AtticaResolverAccount* >( acct ) )
            {
                if ( resolver->atticaId() == atticaContent.id() )
                {
                    qDebug() << "found atticaaccount :" << resolver->accountFriendlyName();
                    atticaAccount = resolver;
                    break;
                }
            }
        }
    }

    explicit AccountModelNode( AccountModelNode* p, ResolverAccount* ra ) : parent( p ), type( ManualResolverType )
    {
        init();
        resolverAccount = ra;
    }

    AccountModelNode() : parent( 0 ) {}

    ~AccountModelNode()
    {
        qDeleteAll( children );
    }


    void init()
    {
        parent->children.append( this );

        factory = 0;
        account = 0;
        atticaAccount = 0;
        resolverAccount = 0;
    }
};

}

}
#endif // TOMAHAWK_ACCOUNTS_ACCOUNTMODELNODE_H
