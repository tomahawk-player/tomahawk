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

class ResolverAccount;

class DLLEXPORT ResolverAccountFactory : public AccountFactory
{
    Q_OBJECT
public:
    ResolverAccountFactory() {}
    virtual ~ResolverAccountFactory() {}

    Account* createAccount( const QString& accountId = QString() ) override;
    QString factoryId() const override { return "resolveraccount"; }
    QString description() const override { return QString(); }
    QString prettyName() const override { return QString(); } // Internal, not displayed
    AccountTypes types() const override { return AccountTypes( ResolverType ); }
    bool allowUserCreation() const override { return false; }

    // Used to create a new resolver from a script on disk, either chosen by
    // the user, or installed from synchrotron
    bool acceptsPath( const QString&  ) const override { return true; } // This is the catch-all filesystem account
    Account* createFromPath( const QString& path ) override;

    // Internal use
    static ResolverAccount* createFromPath( const QString& path, const QString& factoryId, bool isAttica );
    // YES, non const parameters!
    static bool installAxe( QString& realPath, QVariantHash& configuration );


private:
    static void displayError( const QString& error );
    static void expandPaths( const QDir& contentDir, QVariantHash& configuration );
    static QVariantHash metadataFromJsonFile( const QString& path );
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

    void authenticate() override;
    void deauthenticate() override;
    bool isAuthenticated() const override;
    Tomahawk::Accounts::Account::ConnectionState connectionState() const override;

    AccountConfigWidget* configurationWidget() override;
    QString errorMessage() const override;

    void saveConfig() override;
    void removeFromConfig() override;

    QString path() const;

    QPixmap icon() const override;
    QString description() const override;
    QString author() const override;
    QString version() const override;

    // Not relevant
    SipPlugin* sipPlugin( bool ) override { return nullptr; }
    Tomahawk::InfoSystem::InfoPluginPtr infoPlugin() override { return Tomahawk::InfoSystem::InfoPluginPtr(); }
    QWidget* aclWidget() override { return nullptr; }

    void removeBundle();

    void testConfig() override;

    ExternalResolverGui* resolver() const;

private slots:
    void resolverChanged();
    void onTestConfig( const QVariant& result );

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

    QPixmap icon() const override;

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
