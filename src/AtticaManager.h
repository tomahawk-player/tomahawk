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

#ifndef ATTICAMANAGER_H
#define ATTICAMANAGER_H

#include <QObject>
#include <QHash>

#ifdef LIBATTICA_FOUND
#include <attica/provider.h>
#include <attica/providermanager.h>
#include <attica/content.h>
#endif

class AtticaManager : public QObject
{
    Q_OBJECT
public:
    enum ResolverState {
        Uninstalled = 0,
        Installing,
        Installed,
        NeedsUpgrade,
        Upgrading,
        Failed
    };

    typedef QHash< QString, AtticaManager::ResolverState > StateHash;

    static AtticaManager* instance() {
        if ( !s_instance )
            s_instance = new AtticaManager();

        return s_instance;
    }

    explicit AtticaManager ( QObject* parent = 0 );
    virtual ~AtticaManager() {}

#ifdef LIBATTICA_FOUND
    bool resolversLoaded() const;

    Attica::Content::List resolvers() const;
    ResolverState resolverState( const Attica::Content& resolver ) const;

    void installResolver( const Attica::Content& resolver );
    void uninstallResolver( const Attica::Content& resolver );
    QString pathFromId( const QString& resolverId ) const;

signals:
    void resolversReloaded( const Attica::Content::List& resolvers );

    void resolverStateChanged( const QString& resolverId );
    void resolverInstalled( const QString& resolverId );
    void resolverUninstalled( const QString& resolverId );

private slots:
    void providerAdded( const Attica::Provider& );
    void resolversList( Attica::BaseJob* );
    void resolverDownloadFinished( Attica::BaseJob* );
    void payloadFetched();

private:
    QString extractPayload( const QString& filename, const QString& resolverId ) const;
    bool removeDirectory( const QString& dir ) const;

    Attica::ProviderManager m_manager;

    Attica::Provider m_resolverProvider;
    Attica::Content::List m_resolvers;
    StateHash m_resolverStates;
#endif

    static AtticaManager* s_instance;
};

#endif // ATTICAMANAGER_H
