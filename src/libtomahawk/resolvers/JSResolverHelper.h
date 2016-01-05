/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef JSRESOLVERHELPER_H
#define JSRESOLVERHELPER_H

#include "DllMacro.h"
#include "Typedefs.h"
#include "UrlHandler.h"
#include "database/fuzzyindex/FuzzyIndex.h"
#include "utils/NetworkReply.h"

#include <QObject>
#include <QVariantMap>

#include <functional>



Q_DECLARE_METATYPE( std::function< void( QSharedPointer< QIODevice >& ) >  )

namespace Tomahawk
{

class JSResolver;

class DLLEXPORT JSResolverHelper : public QObject
{
Q_OBJECT

public:
    JSResolverHelper( const QString& scriptPath, JSResolver* parent );

    /**
     * INTERNAL USE ONLY!
     */
    void setResolverConfig( const QVariantMap& config );

    void start();
    void stop();

    /**
     * Get the instance unique account id for this resolver.
     *
     * INTERNAL USE ONLY!
     */
    Q_INVOKABLE QString accountId();


    /**
     * Make Tomahawk assert the assertion is true, probably not to be used by resolvers directly
     */
    Q_INVOKABLE void nativeAssert( bool assertion, const QString& message = QString() );

    /**
     * Retrieve metadata for a media stream.
     *
     * Current suported transport protocols are:
     *  * HTTP
     *  * HTTPS
     *
     * This method is asynchronous and will call
     *     Tomahawk.retrievedMetadata(metadataId, metadata, error)
     * on completion. This method is an internal variant, JavaScript resolvers
     * are advised to use Tomahawk.retrieveMetadata(url, options, callback).
     *
     * INTERNAL USE ONLY!
     */
    Q_INVOKABLE void nativeRetrieveMetadata( int metadataId, const QString& url,
                                             const QString& mimetype,
                                             int sizehint,
                                             const QVariantMap& options );

    Q_INVOKABLE void invokeNativeScriptJob( int requestId,
                                        const QString& methodName,
                                        const QVariantMap& params );

    /**
     * Lucene++ indices for JS resolvers
     **/

    Q_INVOKABLE bool hasFuzzyIndex();
    Q_INVOKABLE void createFuzzyIndex( const QVariantList& list );
    Q_INVOKABLE void addToFuzzyIndex( const QVariantList& list );
    Q_INVOKABLE QVariantList searchFuzzyIndex( const QString& query );
    Q_INVOKABLE QVariantList resolveFromFuzzyIndex( const QString& artist, const QString& album, const QString& tracks );
    Q_INVOKABLE void deleteFuzzyIndex();

    /**
     * This is horrible, we can use it to invalidate resolver results when the config changes
     * TODO: register the resolver through registerPlugin and remove it through standard methods
     */
    Q_INVOKABLE void readdResolver();


public slots:
    QByteArray readRaw( const QString& fileName );
    QString readBase64( const QString& fileName );
    QString readCompressed( const QString& fileName );
    QString uuid() const;
    int currentCountry() const;
    QString compress( const QString& data );
    QVariantMap resolverData();

    void log( const QString& message );
    bool fakeEnv() { return false; }

    void nativeReportCapabilities( const QVariant& capabilities );

    void reportScriptJobResults( const QVariantMap& result );

    void registerScriptPlugin( const QString& type, const QString& objectId );
    void unregisterScriptPlugin( const QString& type, const QString& objectId );

private slots:
    void nativeAsyncRequestDone( int requestId, NetworkReply* reply );

private:
    bool indexDataFromVariant( const QVariantMap& map, struct Tomahawk::IndexData& indexData );
    QVariantList searchInFuzzyIndex( const Tomahawk::query_ptr& query );

    // native script jobs
    void nativeAsyncRequest( int requestId, const QVariantMap& options );


    QVariantMap m_resolverConfig;
    JSResolver* m_resolver;
    QString m_scriptPath;
    bool m_stopped;
};

} // ns: Tomahawk

#endif // JSRESOLVERHELPER_H
