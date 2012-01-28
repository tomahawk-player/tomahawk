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

#ifndef RESOLVERACCOUNT_H
#define RESOLVERACCOUNT_H

#include "accounts/Account.h"

namespace Tomahawk {

class ExternalResolverGui;

namespace Accounts {

class ResolverAccountFactory : public AccountFactory
{
    Q_OBJECT
public:
    ResolverAccountFactory() {}
    virtual ~ResolverAccountFactory() {}

    virtual Account* createAccount(const QString& accountId = QString());
    virtual QString factoryId() const  { return "resolveraccount"; }
    virtual QString description() const { return QString(); }
    virtual QString prettyName() const { return QString(); } // Internal, not displayed
    virtual bool allowUserCreation() const { return false; }

    // Used to create a new resolver from a script on disk, either chosen by
    // the user, or installed from synchrotron
    static Account* createFromPath( const QString& path, bool isAttica );
};

/**
 * Helper wrapper class that is a resolver-only account.
 *
 * Contains the resolver* that is it wrapping
 */
class ResolverAccount : public Account
{
    Q_OBJECT
public:
    explicit ResolverAccount( const QString& accountId );
    virtual ~ResolverAccount();

    virtual void authenticate();
    virtual void deauthenticate();
    virtual bool isAuthenticated() const;
    virtual Tomahawk::Accounts::Account::ConnectionState connectionState() const;

    virtual QWidget* configurationWidget();
    virtual QString errorMessage() const;

    virtual void saveConfig();
    virtual void removeFromConfig();

    QString path() const;

    // Not relevant
    virtual QPixmap icon() const { return QPixmap(); }
    virtual SipPlugin* sipPlugin() { return 0; }
    virtual Tomahawk::InfoSystem::InfoPlugin* infoPlugin() { return 0; }
    virtual QWidget* aclWidget() { return 0; }

private slots:
    void resolverChanged();

protected:
    ResolverAccount( const QString& accountId, const QString& path );
    ExternalResolverGui* m_resolver;

    friend class ResolverAccountFactory;
};


/**
 * Extends ResolverAccount with what attica additionally provides---e.g. icon
 * Assumes certain file layout on disk.
 */
class AtticaResolverAccount : public ResolverAccount
{
    Q_OBJECT
public:
    explicit AtticaResolverAccount(const QString& accountId);
    virtual ~AtticaResolverAccount();

    virtual QPixmap icon() const;

private:
    AtticaResolverAccount( const QString& accountId, const QString& path );

    void loadIcon();

    QPixmap m_icon;

    friend class ResolverAccountFactory;
};

}
}

#endif // RESOLVERACCOUNT_H
