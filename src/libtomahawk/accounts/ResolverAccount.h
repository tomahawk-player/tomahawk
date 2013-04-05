/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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
#include "DllMacro.h"

#include <QPointer>

class QDir;

namespace Tomahawk {

class ExternalResolverGui;

namespace Accounts {

class DLLEXPORT ResolverAccountFactory : public AccountFactory
{
    Q_OBJECT
public:
    ResolverAccountFactory() {}
    virtual ~ResolverAccountFactory() {}

    virtual Account* createAccount( const QString& accountId = QString() );
    virtual QString factoryId() const  { return "resolveraccount"; }
    virtual QString description() const { return QString(); }
    virtual QString prettyName() const { return QString(); } // Internal, not displayed
    AccountTypes types() const { return AccountTypes( ResolverType ); };
    virtual bool allowUserCreation() const { return false; }

    // Used to create a new resolver from a script on disk, either chosen by
    // the user, or installed from synchrotron
    virtual bool acceptsPath( const QString&  ) const { return true; } // This is the catch-all filesystem account
    virtual Account* createFromPath( const QString& path );

    // Internal use
    static Account* createFromPath( const QString& path, const QString& factoryId, bool isAttica );

private:
    static QVariantHash metadataFromJsonFile( const QString& path );
    static void expandPaths( const QDir& contentDir, QVariantHash& configuration );
};

/**
 * Helper wrapper class that is a resolver-only account.
 *
 * Contains the resolver* that is it wrapping
 */
class DLLEXPORT ResolverAccount : public Account
{
    Q_OBJECT
public:
    // Loads from config. Must already exist.
    explicit ResolverAccount( const QString& accountId );
    virtual ~ResolverAccount();

    virtual void authenticate();
    virtual void deauthenticate();
    virtual bool isAuthenticated() const;
    virtual Tomahawk::Accounts::Account::ConnectionState connectionState() const;

    virtual AccountConfigWidget* configurationWidget();
    virtual QString errorMessage() const;

    virtual void saveConfig();
    virtual void removeFromConfig();

    QString path() const;

    virtual QPixmap icon() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QString version() const;

    // Not relevant
    virtual SipPlugin* sipPlugin() { return 0; }
    virtual Tomahawk::InfoSystem::InfoPluginPtr infoPlugin() { return Tomahawk::InfoSystem::InfoPluginPtr(); }
    virtual QWidget* aclWidget() { return 0; }

    virtual void removeBundle();

private slots:
    void resolverChanged();

protected:
    // Created by factory, when user installs a new resolver
    ResolverAccount( const QString& accountId, const QString& path, const QVariantHash& initialConfiguration = QVariantHash() );
    void hookupResolver();

    QPointer<ExternalResolverGui> m_resolver;

private:
    void init( const QString& path );

    friend class ResolverAccountFactory;
};


/**
 * Extends ResolverAccount with what attica additionally provides---e.g. icon
 * Assumes certain file layout on disk.
 */
class DLLEXPORT AtticaResolverAccount : public ResolverAccount
{
    Q_OBJECT
public:
    // Loads from config
    explicit AtticaResolverAccount( const QString& accountId );
    virtual ~AtticaResolverAccount();

    virtual QPixmap icon() const;

    QString atticaId() const { return m_atticaId; }

    void setPath( const QString& path );

private slots:
    void resolverIconUpdated( const QString& );

    void loadIcon();

private:
    // Created by factory, when user installs a new resolver
    AtticaResolverAccount( const QString& accountId, const QString& path, const QString& atticaId, const QVariantHash& initialConfiguration = QVariantHash() );

    void init();

    QPixmap m_icon;
    QString m_atticaId;

    friend class ResolverAccountFactory;
};

}
}

#endif // RESOLVERACCOUNT_H
